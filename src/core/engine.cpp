#include "engine.h"

#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace core
{
  Engine& Engine::singleton()
  {
    static Engine engine;

    return engine;
  }

  void Engine::init()
  {
    if (!SDL_Init(SDL_INIT_VIDEO))
      throw std::runtime_error(SDL_GetError());

    m_window = SDL_CreateWindow("Vulkan", 800, 600, SDL_WINDOW_VULKAN);
    if (!m_window)
      throw std::runtime_error(SDL_GetError());

    uint32_t extensionCount = 0;
    auto extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

    VkResult result = VK_SUCCESS;
    VkApplicationInfo appInfo = {};
    VkInstanceCreateInfo instanceCreateInfo = {};

    // A generic application info structure
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Application";
    appInfo.applicationVersion = 1;
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

    // Create the instance.
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = extensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = extensionNames;

    result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);

    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create vulkan instance");

    // First figure out how many devices are in the system.
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);

    if (physicalDeviceCount == 0)
      throw std::runtime_error("No physical device available");

    // Size the device array appropriately and get the physical device handles.
    m_physicalDevices.resize(physicalDeviceCount);
    vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount,
                               &m_physicalDevices[0]);

    VkPhysicalDeviceFeatures supportedFeatures;
    VkPhysicalDeviceFeatures requiredFeatures = {};

    vkGetPhysicalDeviceFeatures(m_physicalDevices[0], &supportedFeatures);

    requiredFeatures.multiDrawIndirect = supportedFeatures.multiDrawIndirect;
    requiredFeatures.tessellationShader = VK_TRUE;
    requiredFeatures.geometryShader = VK_TRUE;

    // clang-format off
    const VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
      nullptr,                                    // pNext
      0,                                          // flags
      0,                                          // queueFamilyIndex
      1,                                          // queueCount
      nullptr                                     // pQueuePriorities
    };

    const VkDeviceCreateInfo deviceCreateInfo = {
      VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, // sType
      nullptr,                              // pNext
      0,                                    // flags
      1,                                    // queueCreateInfoCount
      &deviceQueueCreateInfo,               // pQueueCreateInfos
      0,                                    // enabledLayerCount
      nullptr,                              // ppEnabledLayerNames
      0,                                    // enabledExtensionCount
      nullptr,                              // ppEnabledExtensionNames
      &requiredFeatures                     // pEnabledFeatures
    };
    // clang-format on

    result = vkCreateDevice(m_physicalDevices[0], &deviceCreateInfo, nullptr,
                            &m_logicalDevice);

    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create logical device");

    if (!SDL_Vulkan_CreateSurface(m_window, m_instance, nullptr, &m_surface))
      throw std::runtime_error("Failed to create vulkan surface");
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
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);

    SDL_DestroyWindow(m_window);
    SDL_Quit();
  }
} // namespace core
