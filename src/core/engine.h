#pragma once

#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

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

    uint32_t chooseHeapFromFlags(const VkMemoryRequirements& memoryRequirements,
                                 VkMemoryPropertyFlags requiredFlags,
                                 VkMemoryPropertyFlags preferredFlags) const;

    void loadShader(const std::string& filename, VkShaderModule* shaderModule);

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
    VkSemaphore m_imageAvailableSemaphore;
    VkSemaphore m_renderFinishedSemaphore;
    VkFence m_inFlightFence;
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkShaderModule m_vertexShaderModule;
    VkShaderModule m_fragmentShaderModule;

    SDL_Window* m_window;
  };
} // namespace core
