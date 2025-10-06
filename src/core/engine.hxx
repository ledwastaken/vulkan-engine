#include "core/engine.h"

namespace core
{
  inline SDL_Window* Engine::get_window() const { return window_; }
  inline VkDevice Engine::get_device() const { return device_; }
  inline VkExtent2D Engine::get_swapchain_extent() const { return swapchain_extent_; }
} // namespace core
