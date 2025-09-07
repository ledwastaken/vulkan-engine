#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>

namespace core
{
  class Engine
  {
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    Engine() = default;

  public:
    static Engine& singleton();

    void init();
    void loop();
    void quit();

  private:
    VkInstance m_instance;
    std::vector<VkPhysicalDevice> m_physicalDevices;
    VkDevice m_logicalDevice;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    VkRenderPass m_renderpass;
    std::vector<VkFramebuffer> m_framebuffers;
    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;

    SDL_Window* m_window;
  };
} // namespace core
