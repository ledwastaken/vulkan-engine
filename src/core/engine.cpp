#include "engine.h"

#include <cstring>
#include <fstream>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

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
    create_swapchain_image_views_and_frambuffers();
    create_pipeline_layout();
    create_graphics_pipeline();
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
        {
          replace_swapchain();
          create_swapchain_image_views_and_frambuffers();
        }
      }
    }
  }

  void Engine::quit()
  {
    vkDeviceWaitIdle(device_);

    vkDestroyShaderModule(device_, vertex_shader_, nullptr);
    vkDestroyShaderModule(device_, fragment_shader_, nullptr);

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

    // TODO: Check device's image count capabilities

    const VkSwapchainCreateInfoKHR create_info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext = nullptr,
      .flags = 0,
      .surface = surface_,
      .minImageCount = 2,
      .imageFormat = VK_FORMAT_R8G8B8A8_UNORM,
      .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
      .imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) },
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

  void Engine::create_swapchain_image_views_and_frambuffers()
  {
    int width, height;
    if (!SDL_GetWindowSize(window_, &width, &height))
      throw std::runtime_error(SDL_GetError());

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
        .image = swapchain_images_[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .components = components,
        .subresourceRange = subresource_range,
      };

      result =
          vkCreateImageView(device_, &image_view_create_info, nullptr, &swapchain_image_views_[i]);

      if (result != VK_SUCCESS)
        throw std::runtime_error("failed to create image view");

      const VkImageView attachments[] = { swapchain_image_views_[i] };

      const VkFramebufferCreateInfo framebuffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderPass = renderpass_,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .layers = 1,
      };

      result = vkCreateFramebuffer(device_, &framebuffer_create_info, nullptr, &framebuffers_[i]);

      if (result != VK_SUCCESS)
        throw std::runtime_error("failed to create frambuffer");
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

  void Engine::create_graphics_pipeline()
  {
    create_shader_module("shader.vert.spv", &vertex_shader_);
    create_shader_module("shader.frag.spv", &fragment_shader_);

    // FIXME
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

  void Engine::replace_swapchain()
  {
    if (vkDeviceWaitIdle(device_) != VK_SUCCESS)
      throw std::runtime_error("failed to wait idle for device");

    // TODO: Delete framebuffers

    for (size_t i = 0; i < swapchain_image_views_.size(); i++)
      vkDestroyImageView(device_, swapchain_image_views_[i], nullptr);

    vkDestroySwapchainKHR(device_, swapchain_, nullptr);

    create_swapchain();
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
} // namespace core
