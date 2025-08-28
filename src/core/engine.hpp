#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace core
{
  class Engine
  {
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    Engine() = default;
    ~Engine();

  public:
    static Engine& get();

    VkResult init();
    void run();

  private:
    VkInstance m_instance;
    std::vector<VkPhysicalDevice> m_physicalDevices;
    VkDevice m_logicalDevice;
  };
} // namespace core
