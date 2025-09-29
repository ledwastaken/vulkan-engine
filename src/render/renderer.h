#pragma once

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "scene/visitor.h"

namespace render
{
  class Renderer : public scene::Visitor
  {
  public:
    using Visitor::operator();

    void operator()(scene::Scene& scene, uint32_t image_index);
    void operator()(scene::Mesh& mesh) override;

  private:
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
  };
} // namespace render
