#pragma once

#include <ktx.h>

#include "gfx/renderer.h"
#include "misc/singleton.h"
#include "types/matrix4.h"

namespace gfx
{
  class SkyboxRenderer
    : public misc::Singleton<SkyboxRenderer>
    , public Renderer
  {
    // Give Singleton<SkyboxRenderer> access to class’s private constructor
    friend class Singleton<SkyboxRenderer>;

  private:
    SkyboxRenderer() = default;

  public:
    void init();
    void draw(VkImageView image_view, VkCommandBuffer command_buffer, const types::Matrix4& view,
              const types::Matrix4& projection);
    void free();

  private:
    void create_pipeline_layout();
    void create_pipeline_cache();
    void create_graphics_pipeline();
    void create_vertex_buffer();

    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkPipelineCache pipeline_cache_ = VK_NULL_HANDLE;
    VkShaderModule vertex_shader_ = VK_NULL_HANDLE;
    VkShaderModule fragment_shader_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkBuffer vertex_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertex_buffer_memory_ = VK_NULL_HANDLE;
  };
} // namespace gfx

#include "gfx/skybox-renderer.hxx"
