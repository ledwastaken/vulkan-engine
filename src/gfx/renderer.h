#pragma once

#include <vulkan/vulkan.h>

namespace gfx
{
  class Renderer
  {
  protected:
    virtual void create_shader_module(const char* path, VkShaderModule* shader_module);
  };
} // namespace gfx
