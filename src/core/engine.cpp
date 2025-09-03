#include "engine.h"

namespace core
{
  Engine::~Engine() { vkDestroyInstance(m_instance, nullptr); }

  Engine& Engine::get()
  {
    static Engine engine;

    return engine;
  }

  bool Engine::init()
  {
    if (initVulkan() != VK_SUCCESS)
      return false;
  }

  void Engine::run()
  {
    // FIXME
  }

  VkResult Engine::initVulkan()
  {
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

    result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);

    if (result == VK_SUCCESS)
    {
      // First figure out how many devices are in the system.
      uint32_t physicalDeviceCount = 0;
      vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);

      // Size the device array appropriately and get the physical
      // device handles.
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
    }

    return result;
  };

  bool Engine::initSDL()
  {
    // FIXME
  }
} // namespace core
