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

    void create_swapchain();
    void create_swapchain_image_views();

    void choose_physical_device(std::vector<VkPhysicalDevice> physical_devices);
    bool required_queue_families_not_spported(VkPhysicalDevice device);
    bool required_extensions_not_supported(VkPhysicalDevice device);
    bool swapchain_not_spported(VkPhysicalDevice device);
    bool required_features_not_supported(VkPhysicalDevice device);
    int calculate_device_properties_score(VkPhysicalDeviceProperties properties);
    void replace_swapchain();

    SDL_Window* window_;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    uint32_t graphics_queue_family_;
    uint32_t present_queue_family_;
    VkPhysicalDeviceFeatures enabled_features_ = {};
    VkDevice device_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;
  };
} // namespace core
