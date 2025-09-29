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

    uint32_t find_memory_type(uint32_t required_memory_type, VkMemoryPropertyFlags flags);
    VkCommandBuffer get_command_buffer(uint32_t image_index) const;
    VkFramebuffer get_frambuffer(uint32_t image_index) const;
    VkDevice get_device() const;
    VkExtent2D get_swapchain_extent() const;

  private:
    void create_window();
    void create_instance();
    void create_surface();
    void create_device();

    void create_swapchain();
    void create_renderpass();
    void create_swapchain_resources();
    void create_pipeline_layout();
    void create_pipeline_cache();
    void create_graphics_pipeline();
    void create_command_pools();
    void allocate_command_buffers();
    void create_fences();
    void create_semaphores();

    void choose_physical_device(std::vector<VkPhysicalDevice> physical_devices);
    bool required_queue_families_not_spported(VkPhysicalDevice device);
    bool required_extensions_not_supported(VkPhysicalDevice device);
    bool swapchain_not_spported(VkPhysicalDevice device);
    bool required_features_not_supported(VkPhysicalDevice device);
    int calculate_device_properties_score(VkPhysicalDeviceProperties properties);
    void create_image_view(size_t index);
    void create_framebuffer(size_t index);
    void replace_swapchain();
    void create_shader_module(const char* path, VkShaderModule* shader_module);

    SDL_Window* window_;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    uint32_t graphics_queue_family_;
    uint32_t present_queue_family_;
    VkPhysicalDeviceFeatures enabled_features_ = {};
    VkDevice device_ = VK_NULL_HANDLE;
    VkExtent2D swapchain_extent_;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;
    VkRenderPass renderpass_;
    std::vector<VkFramebuffer> framebuffers_;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkPipelineCache pipeline_cache_ = VK_NULL_HANDLE;
    VkShaderModule vertex_shader_ = VK_NULL_HANDLE;
    VkShaderModule fragment_shader_ = VK_NULL_HANDLE;
    VkPipeline graphics_pipeline_ = VK_NULL_HANDLE;
    VkCommandPool graphics_command_pool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> graphics_command_buffers_;
    VkFence in_flight_fence_ = VK_NULL_HANDLE;
    VkSemaphore image_available_semaphore_ = VK_NULL_HANDLE;
    VkSemaphore render_finished_semaphore_ = VK_NULL_HANDLE;
  };
} // namespace core

#include "core/engine.hxx"
