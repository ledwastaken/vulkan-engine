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
    }
  }

  void Engine::quit()
  {
    vkDestroySwapchainKHR(device_, swpachain_, nullptr);
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
      .oldSwapchain = swpachain_,
    };

    if (vkCreateSwapchainKHR(device_, &create_info, nullptr, &swpachain_))
      throw std::runtime_error("failed to create swapchain");
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

    vkDestroySwapchainKHR(device_, swpachain_, nullptr);

    // TODO: Delete framebuffers and imageviews

    create_swapchain();
  }
} // namespace core
