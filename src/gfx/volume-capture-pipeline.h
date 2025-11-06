#pragma once

#include "gfx/pipeline.h"
#include "misc/singleton.h"
#include "scene/mesh.h"

namespace gfx
{
  class VolumeCapturePipeline
    : public misc::Singleton<VolumeCapturePipeline>
    , public Pipeline
  {
    // Give Singleton access to class’s private constructor
    friend class Singleton<VolumeCapturePipeline>;

  private:
    VolumeCapturePipeline() = default;

  public:
    void init();
    void draw(VkImage position_cubemap, VkCommandBuffer command_buffer, scene::Mesh& mesh);
    void free();

  private:
    void create_pipeline_layout();
    void create_pipeline_cache();
    void create_graphics_pipeline();

    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkPipelineCache pipeline_cache_ = VK_NULL_HANDLE;
    VkShaderModule vertex_shader_ = VK_NULL_HANDLE;
    VkShaderModule fragment_shader_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
  };
} // namespace gfx
