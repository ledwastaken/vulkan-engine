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
      }
    }
  }

  void Engine::quit()
  {
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

    choose_physical_device(physical_devices);

    // FIXME
    VkPhysicalDeviceFeatures supported_features;
    VkPhysicalDeviceFeatures required_features = {};

    vkGetPhysicalDeviceFeatures(physical_device_, &supported_features);

    required_features.multiDrawIndirect = supported_features.multiDrawIndirect;
    required_features.tessellationShader = VK_TRUE;
    required_features.geometryShader = VK_TRUE;

    const char* const swapchain_extension[] = {
      "VK_KHR_swapchain",
    };

    const VkDeviceQueueCreateInfo queue_create_info = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .queueFamilyIndex = 0,
      .queueCount = 1,
      .pQueuePriorities = nullptr,
    };

    const VkDeviceCreateInfo deviceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queue_create_info,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = 1,
      .ppEnabledExtensionNames = swapchain_extension,
      .pEnabledFeatures = &required_features,
    };

    result = vkCreateDevice(physical_device_, &deviceCreateInfo, nullptr, &device_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create logical device");
  }

  void Engine::choose_physical_device(std::vector<VkPhysicalDevice> devices)
  {
    int max_score = -1;

    for (int i = 0; i < devices.size(); i++)
    {
      int graphics_family = -1;
      int present_family = -1;

      int score = calculate_device_score(devices[i], &graphics_family, &present_family);
      if (score > max_score)
      {
        physical_device_ = devices[i];
        max_score = score;
      }
    }

    if (max_score < 0)
      throw std::runtime_error("no physical device fits minimum requirements");
  }

  int Engine::calculate_device_score(VkPhysicalDevice device, int* graphics_family,
                                     int* present_family)
  {
    if (required_queue_families_not_spported(device, graphics_family, present_family))
      return -1;

    if (required_extensions_not_supported(device))
      return -1;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory_properties;

    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);
    vkGetPhysicalDeviceMemoryProperties(device, &memory_properties);

    return 0;
  }

  bool Engine::required_queue_families_not_spported(VkPhysicalDevice device, int* graphics_family,
                                                    int* present_family)
  {
    *graphics_family = -1;
    *present_family = -1;

    uint32_t family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, NULL);

    auto family_properties = new VkQueueFamilyProperties[family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, family_properties);

    for (int idx = 0; idx < family_count; idx++)
    {
      VkBool32 supported;
      VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, idx, surface_, &supported);
      if (result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve KHR surface support");

      if (supported)
        *present_family = idx;
      if (family_properties[idx].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        *graphics_family = idx;
      if (*present_family > 0 && *graphics_family > 0)
        break;
    }

    delete family_properties;

    return *present_family == -1 || *graphics_family == -1;
  }

  bool Engine::required_extensions_not_supported(VkPhysicalDevice device)
  {
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    auto extensions = new VkExtensionProperties[extension_count];
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions);

    for (int i = 0; i < extension_count; i++)
    {
      if (strcmp(extensions[i].extensionName, "VK_KHR_swapchain") == 0)
      {
        delete extensions;
        return false;
      }
    }

    delete extensions;
    return true;
  }
} // namespace core
