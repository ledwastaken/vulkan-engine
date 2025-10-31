#pragma once

#include <vulkan/vulkan.h>

#include "misc/singleton.h"
#include "scene/visitor.h"
#include "types/matrix4.h"

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
    using Visitor::operator();

    void init();
    void draw(VkImageView image_view, VkCommandBuffer command_buffer);
    void free();

    void operator()(scene::Mesh& mesh) override;

  private:
    VkImage albedo_image_ = VK_NULL_HANDLE;
    VkImage normal_image_ = VK_NULL_HANDLE;
    VkImage depth_image_ = VK_NULL_HANDLE;

    VkImageView image_view_ = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
    types::Matrix4 view_;
    types::Matrix4 projection_;
  };
} // namespace render
