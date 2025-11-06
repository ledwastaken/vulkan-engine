#pragma once

#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>

#include "misc/singleton.h"

#define MAX_FRAMES_IN_FLIGHT 3

namespace core
{
  struct TransitionLayout
  {
    VkAccessFlags src_access;
    VkAccessFlags dst_access;
    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;
    VkImageAspectFlags aspect_mask;
    VkImageLayout old_layout;
    VkImageLayout new_layout;
  };

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

    void create_buffer(const VkBufferCreateInfo& create_info, VkMemoryPropertyFlags properties,
                       VkBuffer& buffer, VkDeviceMemory& memory) const;
    void create_image(const VkImageCreateInfo& image_create_info, VkMemoryPropertyFlags properties,
                      VkImage& image, VkDeviceMemory& memory);
    uint32_t find_memory_type(uint32_t required_memory_type, VkMemoryPropertyFlags flags) const;
    void transfer_image(VkImage image, VkOffset3D offset, VkExtent3D extent, uint32_t layer_count,
                        VkBuffer buffer) const;
    void transition_image_layout(VkImage image, VkFormat format, uint32_t layer_count,
                                 TransitionLayout transition_layout) const;

    SDL_Window* get_window() const;
    VkDevice get_device() const;
    VkExtent2D get_swapchain_extent() const;
    VkSurfaceFormatKHR get_surface_format() const;
    uint32_t get_current_frame() const;

  private:
    void create_window();
    void create_instance();
    void create_surface();
    void create_device();
    void create_swapchain();
    void create_swapchain_resources();
    void create_command_pools();
    void allocate_command_buffers();
    void create_fences();
    void create_semaphores();
    void init_imgui();

    void choose_physical_device(std::vector<VkPhysicalDevice> physical_devices);
    bool required_queue_families_not_spported(VkPhysicalDevice device);
    bool required_extensions_not_supported(VkPhysicalDevice device);
    bool swapchain_not_spported(VkPhysicalDevice device);
    bool required_features_not_supported(VkPhysicalDevice device);
    int calculate_device_properties_score(VkPhysicalDeviceProperties properties);
    void create_image_view(size_t index);
    void replace_swapchain();
    void transition_transfer_image_layout(VkImage image, VkFormat format, uint32_t layer_count,
                                          VkImageLayout old_layout, VkImageLayout new_layout) const;

    void render();

    SDL_Window* window_;
    ImGuiContext* context_;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    uint32_t graphics_queue_family_;
    uint32_t present_queue_family_;
    uint32_t transfer_queue_family_;
    VkPhysicalDeviceFeatures enabled_features_ = {};
    VkDevice device_ = VK_NULL_HANDLE;
    VkExtent2D swapchain_extent_;
    VkSurfaceFormatKHR surface_format_;
    uint32_t min_image_count_ = 0;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;
    VkCommandPool graphics_command_pool_ = VK_NULL_HANDLE;
    VkCommandPool transfer_command_pool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> graphics_command_buffers_;
    VkCommandBuffer transfer_command_buffer_;
    std::vector<VkFence> in_flight_fences_;
    VkFence transfer_fence_ = VK_NULL_HANDLE;
    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    VkDescriptorPool imgui_descriptor_pool_ = VK_NULL_HANDLE;
    uint32_t current_frame_ = 0;
  };
} // namespace core

#include "core/engine.hxx"
