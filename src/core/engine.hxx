#include "core/engine.h"

namespace core
{
  inline VkCommandBuffer Engine::get_command_buffer(uint32_t image_index) const
  {
    return graphics_command_buffers_[image_index];
  }

  inline VkDevice Engine::get_device() const { return device_; }
} // namespace core
