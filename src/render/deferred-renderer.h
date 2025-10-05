#pragma once

#include <vulkan/vulkan.h>

#include "misc/singleton.h"
#include "scene/visitor.h"

namespace render
{
  class DeferredRenderer
    : public misc::Singleton<DeferredRenderer>
    , public scene::Visitor
  {
    // Give Singleton<DeferredRenderer> access to class’s private constructor
    friend class Singleton<DeferredRenderer>;

  private:
    /// Construct a DeferredRenderer.
    DeferredRenderer() = default;

  public:
    void init();
    void draw(VkImageView image_view, VkCommandBuffer command_buffer);
    void free();

  public:
    VkImage albedo_image_ = VK_NULL_HANDLE;
    VkImage normal_image_ = VK_NULL_HANDLE;
    VkImage depth_image_ = VK_NULL_HANDLE;
  };
} // namespace render
