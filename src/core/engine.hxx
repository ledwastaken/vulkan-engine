#include "core/engine.h"

namespace core
{
  inline SDL_Window* Engine::get_window() const { return window_; }

  inline VkCommandBuffer Engine::get_command_buffer(uint32_t image_index) const
  {
    return graphics_command_buffers_[image_index];
  }

  inline VkDevice Engine::get_device() const { return device_; }
  inline VkExtent2D Engine::get_swapchain_extent() const { return swapchain_extent_; }
} // namespace core
