#include "core/engine.h"

namespace core
{
  inline VkDevice Engine::get_device() const { return device_; }
} // namespace core
