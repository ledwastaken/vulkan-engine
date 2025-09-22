#pragma once

#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

#include "misc/singleton.h"

namespace core
{
  class Engine : public misc::Singleton<Engine>
  {
    // Give Singleton<Engine> access to Engine’s private constructor
    friend class Singleton<Engine>;

  private:
    /// Construct an Engine.
    Engine() = default;

  public:
    void init(int argc, char* argv[]);
    void loop();
    void quit();

  private:
    void create_window();
    void create_instance();
    void create_surface();
    void create_device();
    void choose_physical_device(std::vector<VkPhysicalDevice> physical_devices);
    bool required_features_not_supported(VkPhysicalDeviceProperties properties,
                                         VkPhysicalDeviceFeatures features,
                                         VkPhysicalDeviceMemoryProperties memory_properties);
    int calculate_device_score(VkPhysicalDeviceProperties properties,
                               VkPhysicalDeviceFeatures features,
                               VkPhysicalDeviceMemoryProperties memory_properties);

    SDL_Window* window_;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
  };
} // namespace core
