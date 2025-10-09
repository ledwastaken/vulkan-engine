#include "engine.h"

#include <cstring>
#include <iostream>
#include <set>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "core/asset-manager.h"
#include "gfx/skybox-pipeline.h"
#include "render/deferred-renderer.h"

namespace core
{
  void Engine::init(int argc, char* argv[])
  {
    create_window();
    create_instance();
    create_surface();
    create_device();
    create_swapchain();
    create_swapchain_resources();
    create_command_pools();
    allocate_command_buffers();
    create_fences();
    create_semaphores();

    auto& skybox_pipeline = gfx::SkyboxPipeline::get_singleton();
    auto& deferred_renderer = render::DeferredRenderer::get_singleton();

    skybox_pipeline.init();
    deferred_renderer.init();
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
    auto& asset_manager = AssetManager::get_singleton();
    auto& skybox_pipeline = gfx::SkyboxPipeline::get_singleton();
    auto& deferred_renderer = render::DeferredRenderer::get_singleton();

    vkDeviceWaitIdle(device_);

    asset_manager.free();
    skybox_pipeline.free();
    deferred_renderer.free();

    vkDestroySemaphore(device_, image_available_semaphore_, nullptr);
    vkDestroySemaphore(device_, render_finished_semaphore_, nullptr);
    vkDestroyFence(device_, transfer_fence_, nullptr);
    vkDestroyFence(device_, in_flight_fence_, nullptr);

    vkFreeCommandBuffers(device_, transfer_command_pool_, 1, &transfer_command_buffer_);
    vkFreeCommandBuffers(device_, graphics_command_pool_, graphics_command_buffers_.size(),
                         graphics_command_buffers_.data());

    vkDestroyCommandPool(device_, transfer_command_pool_, nullptr);
    vkDestroyCommandPool(device_, graphics_command_pool_, nullptr);

    for (size_t i = 0; i < swapchain_image_views_.size(); i++)
      vkDestroyImageView(device_, swapchain_image_views_[i], nullptr);

    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    vkDestroyDevice(device_, nullptr);
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);

    SDL_DestroyWindow(window_);
    SDL_Quit();
  }

  void Engine::create_image(const VkImageCreateInfo& image_create_info,
                            VkMemoryPropertyFlags properties, VkImage& image,
                            VkDeviceMemory& memory)
  {
    VkResult result = vkCreateImage(device_, &image_create_info, nullptr, &image);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create image");

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device_, image, &memory_requirements);

    const VkMemoryAllocateInfo image_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = nullptr,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties),
    };

    result = vkAllocateMemory(device_, &image_allocate_info, nullptr, &memory);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to allocate device memory");

    result = vkBindImageMemory(device_, image, memory, 0);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to bind buffer memory");
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

  void Engine::transfer_image(VkImage image, VkOffset3D offset, VkExtent3D extent,
                              VkBuffer buffer) const
  {
    VkResult result = vkWaitForFences(device_, 1, &transfer_fence_, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to wait for fence");

    result = vkResetFences(device_, 1, &transfer_fence_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to reset fence");

    result = vkResetCommandBuffer(transfer_command_buffer_, 0);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to reset command buffer");

    const VkCommandBufferBeginInfo command_buffer_begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = 0,
      .pInheritanceInfo = nullptr,
    };

    result = vkBeginCommandBuffer(transfer_command_buffer_, &command_buffer_begin_info);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to begin command buffer");

    const VkImageSubresourceLayers image_subresource_layers = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel = 0,
      .baseArrayLayer = 0,
      .layerCount = 1,
    };

    const VkBufferImageCopy region = {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = image_subresource_layers,
      .imageOffset = offset,
      .imageExtent = extent,
    };

    transition_image_layout(image, surface_format_.format, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkCmdCopyBufferToImage(transfer_command_buffer_, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    transition_image_layout(image, surface_format_.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkEndCommandBuffer(transfer_command_buffer_);

    const VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = &transfer_command_buffer_,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = nullptr,
    };

    VkQueue transfer_queue;
    vkGetDeviceQueue(device_, transfer_queue_family_, 0, &transfer_queue);

    result = vkQueueSubmit(transfer_queue, 1, &submit_info, transfer_fence_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to submit to queue");

    result = vkWaitForFences(device_, 1, &transfer_fence_, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to wait for transfer completion");
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

    const char* validation_layers[] = { "VK_LAYER_KHRONOS_validation" };

    const VkApplicationInfo appInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "vulkan-engine",
      .applicationVersion = 1,
      .pEngineName = "vulkan-engine",
      .engineVersion = 1,
      .apiVersion = VK_API_VERSION_1_3,
    };

    const VkInstanceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = 1,
      .ppEnabledLayerNames = validation_layers,
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

    float queue_priority = 1.0f;

    std::set<uint32_t> unique_queue_families = {
      graphics_queue_family_,
      present_queue_family_,
      transfer_queue_family_,
    };

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

    for (auto queue_family : unique_queue_families)
    {
      const VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = queue_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
      };

      queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
      .pNext = nullptr,
      .dynamicRendering = VK_TRUE,
    };

    const VkPhysicalDeviceFeatures2 device_features2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &dynamic_rendering_features,
      .features = enabled_features_,
    };

    const VkDeviceCreateInfo deviceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &device_features2,
      .flags = 0,
      .queueCreateInfoCount = queue_create_infos.size(),
      .pQueueCreateInfos = queue_create_infos.data(),
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = 1,
      .ppEnabledExtensionNames = required_extensions,
      .pEnabledFeatures = nullptr,
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

    VkSurfaceCapabilitiesKHR capabilities;
    VkResult result =
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &capabilities);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to retrieve physical device surface capbilities");

    uint32_t format_count = 0;
    result =
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, nullptr);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to retrieve physical device surface formats");

    std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count,
                                                  surface_formats.data());
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to retrieve physical device surface formats");

    if (format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
      surface_format_ = { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    else
    {
      surface_format_ = surface_formats[0];
      for (const auto& available_format : surface_formats)
      {
        if (available_format.format == VK_FORMAT_R8G8B8A8_UNORM
            && available_format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
        {
          surface_format_ = available_format;
          break;
        }
      }
    }

    const VkSwapchainCreateInfoKHR create_info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext = nullptr,
      .flags = 0,
      .surface = surface_,
      .minImageCount = capabilities.minImageCount,
      .imageFormat = surface_format_.format,
      .imageColorSpace = surface_format_.colorSpace,
      .imageExtent = swapchain_extent_,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .preTransform = capabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR,
      .clipped = VK_TRUE,
      .oldSwapchain = swapchain_,
    };

    result = vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create swapchain");
  }

  void Engine::create_swapchain_resources()
  {
    uint32_t image_count = 0;
    VkResult result = vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);

    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to retrieve swpachain image count");

    swapchain_images_.resize(image_count);
    swapchain_image_views_.resize(image_count);
    result = vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, swapchain_images_.data());

    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to retrieve swpachain images");

    for (uint32_t i = 0; i < image_count; i++)
      create_image_view(i);
  }

  void Engine::create_command_pools()
  {
    const VkCommandPoolCreateInfo graphics_command_pool_create_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = graphics_queue_family_,
    };

    VkResult result = vkCreateCommandPool(device_, &graphics_command_pool_create_info, nullptr,
                                          &graphics_command_pool_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create graphics command pool");

    const VkCommandPoolCreateInfo transfer_command_pool_create_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = transfer_queue_family_,
    };

    result = vkCreateCommandPool(device_, &transfer_command_pool_create_info, nullptr,
                                 &transfer_command_pool_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create graphics command pool");
  }

  void Engine::allocate_command_buffers()
  {
    uint32_t image_count = 0;
    VkResult result = vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);

    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to retrieve swpachain image count");

    const VkCommandBufferAllocateInfo graphics_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = graphics_command_pool_,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = image_count,
    };

    graphics_command_buffers_.resize(image_count);
    result = vkAllocateCommandBuffers(device_, &graphics_allocate_info,
                                      graphics_command_buffers_.data());
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to allocate command buffers");

    const VkCommandBufferAllocateInfo transfer_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = transfer_command_pool_,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };

    result = vkAllocateCommandBuffers(device_, &graphics_allocate_info, &transfer_command_buffer_);
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

    if (vkCreateFence(device_, &create_info, nullptr, &transfer_fence_) != VK_SUCCESS)
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
    bool transfer_family_found = false;

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

      if (family_properties[idx].queueFlags & VK_QUEUE_TRANSFER_BIT)
      {
        transfer_queue_family_ = idx;
        transfer_family_found = true;
      }

      if (graphics_family_found && present_family_found && transfer_family_found)
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
      .format = surface_format_.format,
      .components = components,
      .subresourceRange = subresource_range,
    };

    VkResult result = vkCreateImageView(device_, &image_view_create_info, nullptr,
                                        &swapchain_image_views_[index]);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create image view");
  }

  void Engine::replace_swapchain()
  {
    if (vkDeviceWaitIdle(device_) != VK_SUCCESS)
      throw std::runtime_error("failed to wait idle for device");

    for (size_t i = 0; i < swapchain_image_views_.size(); i++)
      vkDestroyImageView(device_, swapchain_image_views_[i], nullptr);

    vkDestroySwapchainKHR(device_, swapchain_, nullptr);

    create_swapchain();
    create_swapchain_resources();
  }

  void Engine::transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout,
                                       VkImageLayout new_layout) const
  {
    VkAccessFlags src_access;
    VkAccessFlags dst_access;
    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED
        && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
      src_access = 0;
      dst_access = VK_ACCESS_TRANSFER_WRITE_BIT;

      src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
             && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
      src_access = VK_ACCESS_TRANSFER_WRITE_BIT;
      dst_access = VK_ACCESS_SHADER_READ_BIT;

      src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
      throw std::invalid_argument("unsupported layout transition!");

    const VkImageSubresourceRange subresource_range = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    };

    const VkImageMemoryBarrier image_memory_barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = src_access,
      .dstAccessMask = dst_access,
      .oldLayout = old_layout,
      .newLayout = new_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = subresource_range,
    };

    vkCmdPipelineBarrier(transfer_command_buffer_, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr,
                         1, &image_memory_barrier);
  }

  void Engine::render()
  {
    uint32_t image_index;
    VkResult result = vkWaitForFences(device_, 1, &in_flight_fence_, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to wait for fences");

    result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, image_available_semaphore_,
                                   VK_NULL_HANDLE, &image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
      replace_swapchain();
      return;
    }
    else if (result != VK_SUCCESS)
      throw std::runtime_error("failed to acquire next image");

    result = vkResetFences(device_, 1, &in_flight_fence_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to reset fences");

    auto& deferred_renderer = render::DeferredRenderer::get_singleton();
    auto image_view = swapchain_image_views_[image_index];
    auto command_buffer = graphics_command_buffers_[image_index];

    deferred_renderer.draw(image_view, command_buffer);

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
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
      replace_swapchain();
    else if (result != VK_SUCCESS)
      throw std::runtime_error("failed to present");
  }
} // namespace core
