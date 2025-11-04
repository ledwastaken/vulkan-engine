#pragma once

#include <vulkan/vulkan.h>

namespace gfx
{
  class Pipeline
  {
  protected:
    virtual void create_shader_module(const char* filename, VkShaderModule* shader_module);
  };
} // namespace gfx
