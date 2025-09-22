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
    int calculate_device_score(VkPhysicalDevice device, int* graphics_family, int* present_family);
    bool required_queue_families_not_spported(VkPhysicalDevice device, int* graphics_family,
                                              int* present_family);

    SDL_Window* window_;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkQueue graphics_queue_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
  };
} // namespace core
