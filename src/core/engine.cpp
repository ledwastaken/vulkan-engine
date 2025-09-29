#include "engine.h"

#include <cstring>
#include <fstream>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "core/scene-manager.h"

namespace core
{
  void Engine::init(int argc, char* argv[])
  {
    create_window();
    create_instance();
    create_surface();
    create_device();
    create_swapchain();
    create_renderpass();
    create_swapchain_resources();
    create_pipeline_layout();
    create_pipeline_cache();
    create_graphics_pipeline();
    create_command_pools();
    allocate_command_buffers();
    create_fences();
    create_semaphores();
  }

  void Engine::loop()
  {
    bool running = true;
    while (running)
    {
      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
        if (event.type == SDL_EVENT_QUIT)
          running = false;
        else if (event.type == SDL_EVENT_WINDOW_RESIZED)
          replace_swapchain();
      }

      render();
    }
  }

  void Engine::quit()
  {
    vkDeviceWaitIdle(device_);

    vkDestroySemaphore(device_, image_available_semaphore_, nullptr);
    vkDestroySemaphore(device_, render_finished_semaphore_, nullptr);
    vkDestroyFence(device_, in_flight_fence_, nullptr);

    vkFreeCommandBuffers(device_, graphics_command_pool_, graphics_command_buffers_.size(),
                         graphics_command_buffers_.data());

    vkDestroyCommandPool(device_, graphics_command_pool_, nullptr);
    vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
    vkDestroyShaderModule(device_, vertex_shader_, nullptr);
    vkDestroyShaderModule(device_, fragment_shader_, nullptr);

    vkDestroyPipelineCache(device_, pipeline_cache_, nullptr);
    vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);

    for (size_t i = 0; i < framebuffers_.size(); i++)
      vkDestroyFramebuffer(device_, framebuffers_[i], nullptr);

    for (size_t i = 0; i < swapchain_image_views_.size(); i++)
      vkDestroyImageView(device_, swapchain_image_views_[i], nullptr);

    vkDestroyRenderPass(device_, renderpass_, nullptr);
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    vkDestroyDevice(device_, nullptr);
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);

    SDL_DestroyWindow(window_);
    SDL_Quit();
  }

  uint32_t Engine::find_memory_type(uint32_t required_memory_type, VkMemoryPropertyFlags flags)
  {
    VkPhysicalDeviceMemoryProperties device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &device_memory_properties);

    for (uint32_t memory_type = 0; memory_type < VK_MAX_MEMORY_TYPES; memory_type++)
      if (required_memory_type & (1 << memory_type))
      {
        const VkMemoryType& type = device_memory_properties.memoryTypes[memory_type];

        if ((type.propertyFlags & flags) == flags)
          return memory_type;
      }

    throw std::runtime_error("no suitable memory type found");
  }

  void Engine::create_window()
  {
    if (!SDL_Init(SDL_INIT_VIDEO))
      throw std::runtime_error(SDL_GetError());

    auto flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY;
    window_ = SDL_CreateWindow("vulkan-engine", 800, 600, flags);

    if (!window_)
      throw std::runtime_error(SDL_GetError());
  }

  void Engine::create_instance()
  {
    // Query Vulkan extensions required by SDL
    uint32_t extensionCount = 0;
    auto extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

    const VkApplicationInfo appInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "vulkan-engine",
      .applicationVersion = 1,
      .pEngineName = "vulkan-engine",
      .engineVersion = 1,
      .apiVersion = VK_MAKE_VERSION(1, 0, 0),
    };

    const VkInstanceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = extensionCount,
      .ppEnabledExtensionNames = extensionNames,
    };

    // Create the instance.
    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS)
      throw std::runtime_error("failed to create vulkan instance");
  }

  void Engine::create_surface()
  {
    if (!SDL_Vulkan_CreateSurface(window_, instance_, nullptr, &surface_))
      throw std::runtime_error("failed to create vulkan surface");
  }

  void Engine::create_device()
  {
    // First figure out how many devices are in the system.
    uint32_t physical_device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance_, &physical_device_count, nullptr);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to enumerate physical devices");

    if (physical_device_count == 0)
      throw std::runtime_error("no physical device available");

    auto physical_devices = std::vector<VkPhysicalDevice>(physical_device_count);
    result = vkEnumeratePhysicalDevices(instance_, &physical_device_count, physical_devices.data());
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to enumerate physical devices");

    const char* required_extensions[] = {
      "VK_KHR_swapchain",
    };

    choose_physical_device(physical_devices);

    VkPhysicalDeviceFeatures supported_features;
    vkGetPhysicalDeviceFeatures(physical_device_, &supported_features);

    enabled_features_ = {};
    enabled_features_.multiDrawIndirect = supported_features.multiDrawIndirect;
    enabled_features_.geometryShader = VK_TRUE;
    enabled_features_.tessellationShader = VK_TRUE;

    const VkDeviceQueueCreateInfo queue_create_info[] = {
      {
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .queueFamilyIndex = graphics_queue_family_,
          .queueCount = 1,
          .pQueuePriorities = nullptr,
      },
      {
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .queueFamilyIndex = present_queue_family_,
          .queueCount = 1,
          .pQueuePriorities = nullptr,
      },
    };

    const VkDeviceCreateInfo deviceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .queueCreateInfoCount = 2,
      .pQueueCreateInfos = queue_create_info,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = 1,
      .ppEnabledExtensionNames = required_extensions,
      .pEnabledFeatures = &enabled_features_,
    };

    result = vkCreateDevice(physical_device_, &deviceCreateInfo, nullptr, &device_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create logical device");
  }

  void Engine::create_swapchain()
  {
    int width, height;
    if (!SDL_GetWindowSize(window_, &width, &height))
      throw std::runtime_error(SDL_GetError());

    swapchain_extent_ = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

    // TODO: Check device's image count capabilities

    const VkSwapchainCreateInfoKHR create_info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext = nullptr,
      .flags = 0,
      .surface = surface_,
      .minImageCount = 2,
      .imageFormat = VK_FORMAT_R8G8B8A8_UNORM,
      .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
      .imageExtent = swapchain_extent_,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .preTransform = VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR,
      .clipped = VK_TRUE,
      .oldSwapchain = swapchain_,
    };

    if (vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_))
      throw std::runtime_error("failed to create swapchain");
  }

  void Engine::create_renderpass()
  {
    const VkAttachmentDescription attachment = {
      .flags = 0,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    const VkAttachmentReference attachment_reference = {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    const VkSubpassDescription subpass = {
      .flags = 0,
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = 0,
      .pInputAttachments = nullptr,
      .colorAttachmentCount = 1,
      .pColorAttachments = &attachment_reference,
      .pResolveAttachments = nullptr,
      .pDepthStencilAttachment = nullptr,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = nullptr,
    };

    VkRenderPassCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .attachmentCount = 1,
      .pAttachments = &attachment,
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 0,
      .pDependencies = nullptr,
    };

    if (vkCreateRenderPass(device_, &create_info, nullptr, &renderpass_) != VK_SUCCESS)
      throw std::runtime_error("failed to create renderpass");
  }

  void Engine::create_swapchain_resources()
  {
    uint32_t image_count = 0;
    VkResult result = vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);

    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to retrieve swpachain image count");

    swapchain_images_.resize(image_count);
    swapchain_image_views_.resize(image_count);
    framebuffers_.resize(image_count);
    result = vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, swapchain_images_.data());

    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to retrieve swpachain images");

    for (uint32_t i = 0; i < image_count; i++)
    {
      create_image_view(i);
      create_framebuffer(i);
    }
  }

  void Engine::create_pipeline_layout()
  {
    const VkPipelineLayoutCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = 0,
      .pSetLayouts = nullptr,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr,
    };

    if (vkCreatePipelineLayout(device_, &create_info, nullptr, &pipeline_layout_) != VK_SUCCESS)
      throw std::runtime_error("failed to create pipeline layout");
  }

  void Engine::create_pipeline_cache()
  {
    const VkPipelineCacheCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .initialDataSize = 0,
      .pInitialData = nullptr,
    };

    if (vkCreatePipelineCache(device_, &create_info, nullptr, &pipeline_cache_) != VK_SUCCESS)
      throw std::runtime_error("failed to create pipeline cache");
  }

  void Engine::create_graphics_pipeline()
  {
    create_shader_module("shader.vert.spv", &vertex_shader_);
    create_shader_module("shader.frag.spv", &fragment_shader_);

    const VkPipelineShaderStageCreateInfo shader_stage_infos[] = {
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vertex_shader_,
          .pName = "main",
          .pSpecializationInfo = nullptr,
      },
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = fragment_shader_,
          .pName = "main",
          .pSpecializationInfo = nullptr,
      },
    };

    const VkVertexInputBindingDescription vertex_binding_descriptions[] = {
      {
          .binding = 0,
          .stride = 24, // total size of one vertex (aligned)
          .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
      },
    };

    const VkVertexInputAttributeDescription vertex_attribute_descriptions[] = {
      {
          .location = 0,
          .binding = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 0,
      },
      {
          .location = 1,
          .binding = 0,
          .format = VK_FORMAT_R16G16B16_SFLOAT,
          .offset = 12,
      },
      {
          .location = 2,
          .binding = 0,
          .format = VK_FORMAT_R16G16_SFLOAT,
          .offset = 24,
      }
    };

    const VkPipelineVertexInputStateCreateInfo vertex_input_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = vertex_binding_descriptions,
      .vertexAttributeDescriptionCount = 3,
      .pVertexAttributeDescriptions = vertex_attribute_descriptions,
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
    };

    const VkPipelineRasterizationStateCreateInfo rasterization_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .depthBiasConstantFactor = 0.0f,
      .depthBiasClamp = 0.0f,
      .depthBiasSlopeFactor = 0.0f,
      .lineWidth = 1.0f,
    };

    const VkPipelineMultisampleStateCreateInfo multisample_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 0.0f,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE,
    };

    const VkDynamicState dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
    };

    const VkPipelineDynamicStateCreateInfo dynamic_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .dynamicStateCount = 2,
      .pDynamicStates = dynamic_states,
    };

    const VkGraphicsPipelineCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stageCount = 2,
      .pStages = shader_stage_infos,
      .pVertexInputState = &vertex_input_state,
      .pInputAssemblyState = &input_assembly_state,
      .pTessellationState = nullptr,
      .pViewportState = nullptr,
      .pRasterizationState = &rasterization_state,
      .pMultisampleState = &multisample_state,
      .pDepthStencilState = nullptr,
      .pColorBlendState = nullptr,
      .pDynamicState = &dynamic_state,
      .layout = pipeline_layout_,
      .renderPass = renderpass_,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0,
    };

    VkResult result = vkCreateGraphicsPipelines(device_, pipeline_cache_, 1, &create_info, nullptr,
                                                &graphics_pipeline_);

    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create graphics pipeline");
  }

  void Engine::create_command_pools()
  {
    const VkCommandPoolCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = graphics_queue_family_,
    };

    if (vkCreateCommandPool(device_, &create_info, nullptr, &graphics_command_pool_) != VK_SUCCESS)
      throw std::runtime_error("failed to create graphics command pool");

    // TODO: create transfer command pool
  }

  void Engine::allocate_command_buffers()
  {
    uint32_t image_count = 0;
    VkResult result = vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);

    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to retrieve swpachain image count");

    const VkCommandBufferAllocateInfo allocate_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = graphics_command_pool_,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = image_count,
    };

    graphics_command_buffers_.resize(image_count);
    result = vkAllocateCommandBuffers(device_, &allocate_info, graphics_command_buffers_.data());

    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to allocate command buffers");
  }

  void Engine::create_fences()
  {
    const VkFenceCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    if (vkCreateFence(device_, &create_info, nullptr, &in_flight_fence_) != VK_SUCCESS)
      throw std::runtime_error("failed to create fence");
  }

  void Engine::create_semaphores()
  {
    const VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
    };

    VkResult result =
        vkCreateSemaphore(device_, &create_info, nullptr, &image_available_semaphore_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create semaphore");

    result = vkCreateSemaphore(device_, &create_info, nullptr, &render_finished_semaphore_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create semaphore");
  }

  void Engine::choose_physical_device(std::vector<VkPhysicalDevice> devices)
  {
    int max_score = -1;

    for (size_t i = 0; i < devices.size(); i++)
    {
      VkPhysicalDeviceProperties properties;

      vkGetPhysicalDeviceProperties(devices[i], &properties);

      if (required_queue_families_not_spported(devices[i]))
        continue;

      if (required_extensions_not_supported(devices[i]))
        continue;

      if (swapchain_not_spported(devices[i]))
        continue;

      if (required_features_not_supported(devices[i]))
        continue;

      int score = calculate_device_properties_score(properties);
      if (score > max_score)
      {
        physical_device_ = devices[i];
        max_score = score;
      }
    }

    if (max_score < 0)
      throw std::runtime_error("no physical device fits minimum requirements");
  }

  bool Engine::required_queue_families_not_spported(VkPhysicalDevice device)
  {
    uint32_t family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, NULL);

    auto family_properties = std::vector<VkQueueFamilyProperties>(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, family_properties.data());

    bool graphics_family_found = false;
    bool present_family_found = false;

    for (uint32_t idx = 0; idx < family_count; idx++)
    {
      VkBool32 supported;
      VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, idx, surface_, &supported);

      if (result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve KHR surface support");

      if (supported)
      {
        present_queue_family_ = idx;
        present_family_found = true;
      }

      if (family_properties[idx].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        graphics_queue_family_ = idx;
        graphics_family_found = true;
      }

      if (graphics_family_found && present_family_found)
        break;
    }

    return !graphics_family_found || !present_family_found;
  }

  bool Engine::required_extensions_not_supported(VkPhysicalDevice device)
  {
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    auto extensions = std::vector<VkExtensionProperties>(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data());

    for (uint32_t i = 0; i < extension_count; i++)
    {
      if (strcmp(extensions[i].extensionName, "VK_KHR_swapchain") == 0)
        return false;
    }

    return true;
  }

  bool Engine::swapchain_not_spported(VkPhysicalDevice device)
  {
    uint32_t format_count;
    uint32_t present_mode_count;
    VkResult result =
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);

    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to retrieve KHR surface formats");

    result =
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);

    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to retrieve KHR surface present modes");

    return format_count == 0 || present_mode_count == 0;
  }

  bool Engine::required_features_not_supported(VkPhysicalDevice device)
  {
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    return features.geometryShader == VK_FALSE || features.tessellationShader == VK_FALSE;
  }

  int Engine::calculate_device_properties_score(VkPhysicalDeviceProperties properties)
  {
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
      return 4;
    else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
      return 3;
    else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
      return 2;
    else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
      return 1;
    else
      return 0;
  }

  void Engine::create_image_view(size_t index)
  {
    const VkComponentMapping components = {
      .r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    const VkImageSubresourceRange subresource_range = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    };

    const VkImageViewCreateInfo image_view_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = swapchain_images_[index],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .components = components,
      .subresourceRange = subresource_range,
    };

    VkResult result = vkCreateImageView(device_, &image_view_create_info, nullptr,
                                        &swapchain_image_views_[index]);

    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create image view");
  }

  void Engine::create_framebuffer(size_t index)
  {
    const VkImageView attachments[] = { swapchain_image_views_[index] };

    const VkFramebufferCreateInfo framebuffer_create_info = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .renderPass = renderpass_,
      .attachmentCount = 1,
      .pAttachments = attachments,
      .width = swapchain_extent_.width,
      .height = swapchain_extent_.height,
      .layers = 1,
    };

    VkResult result =
        vkCreateFramebuffer(device_, &framebuffer_create_info, nullptr, &framebuffers_[index]);

    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create frambuffer");
  }

  void Engine::replace_swapchain()
  {
    if (vkDeviceWaitIdle(device_) != VK_SUCCESS)
      throw std::runtime_error("failed to wait idle for device");

    for (size_t i = 0; i < framebuffers_.size(); i++)
      vkDestroyFramebuffer(device_, framebuffers_[i], nullptr);

    for (size_t i = 0; i < swapchain_image_views_.size(); i++)
      vkDestroyImageView(device_, swapchain_image_views_[i], nullptr);

    vkDestroySwapchainKHR(device_, swapchain_, nullptr);

    create_swapchain();
    create_swapchain_resources();
  }

  void Engine::create_shader_module(const char* path, VkShaderModule* shader_module)
  {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
      throw std::runtime_error("failed to open file");

    size_t file_size = file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    const VkShaderModuleCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .codeSize = file_size,
      .pCode = reinterpret_cast<uint32_t*>(buffer.data()),
    };

    VkResult result = vkCreateShaderModule(device_, &create_info, nullptr, shader_module);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create shader module");
  }

  void Engine::render()
  {
    auto& scene_manager = SceneManager::get_singleton();
    auto scene = scene_manager.get_current_scene();

    if (!scene)
      return;

    uint32_t image_index;
    VkResult result = vkWaitForFences(device_, 1, &in_flight_fence_, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to wait for fences");

    vkResetFences(device_, 1, &in_flight_fence_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to reset fences");

    vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, image_available_semaphore_,
                          VK_NULL_HANDLE, &image_index);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to acquire next image");

    renderer_(*scene, image_index);

    const VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    const VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &image_available_semaphore_,
      .pWaitDstStageMask = wait_stages,
      .commandBufferCount = 1,
      .pCommandBuffers = &graphics_command_buffers_[image_index],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &render_finished_semaphore_,
    };

    VkQueue graphics_queue;
    vkGetDeviceQueue(device_, graphics_queue_family_, 0, &graphics_queue);

    result = vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fence_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to submit");

    const VkPresentInfoKHR present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = nullptr,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &render_finished_semaphore_,
      .swapchainCount = 1,
      .pSwapchains = &swapchain_,
      .pImageIndices = &image_index,
      .pResults = nullptr,
    };

    VkQueue present_queue;
    vkGetDeviceQueue(device_, present_queue_family_, 0, &present_queue);

    result = vkQueuePresentKHR(present_queue, &present_info);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to present");
  }
} // namespace core
