#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "core/engine.h"
#include "misc/singleton.h"
#include "scene/visitor.h"
#include "types/matrix4.h"

namespace render
{
  class Renderer
    : public misc::Singleton<Renderer>
    , public scene::Visitor
  {
    // Give Singleton access to class’s private constructor
    friend class Singleton<Renderer>;

  private:
    Renderer() = default;

  public:
    using Visitor::operator();

    void init();
    void draw(VkImageView image_view, VkCommandBuffer command_buffer);
    void free();

    void operator()(scene::Mesh& mesh) override;

  private:
    void clear_depth() const;

    VkImage depth_image_ = VK_NULL_HANDLE;
    VkImageView depth_image_view_ = VK_NULL_HANDLE;
    VkSampler depth_sampler_ = VK_NULL_HANDLE;
    VkDeviceMemory depth_image_memory_ = VK_NULL_HANDLE;

    VkImageView image_view_ = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
    types::Matrix4 view_;
    types::Matrix4 projection_;

    std::vector<VkBuffer> uniform_buffers_;
    std::vector<VkDeviceMemory> uniform_buffers_memory_;
    std::vector<void*> uniform_buffers_data_;
  };
} // namespace render
