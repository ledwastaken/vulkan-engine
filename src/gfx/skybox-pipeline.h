#pragma once

#include <vector>

#include <ktx.h>

#include "gfx/pipeline.h"
#include "misc/singleton.h"
#include "types/matrix4.h"

namespace gfx
{
  struct SkyboxData
  {
    VkImage image;
    VkImageView image_view;
    VkSampler sampler;
  };

  class SkyboxPipeline
    : public misc::Singleton<SkyboxPipeline>
    , public Pipeline
  {
    // Give Singleton<SkyboxPipeline> access to class’s private constructor
    friend class Singleton<SkyboxPipeline>;

  private:
    SkyboxPipeline() = default;

  public:
    void init();
    void draw(VkImageView image_view, VkCommandBuffer command_buffer, const types::Matrix4& view,
              const types::Matrix4& projection, const SkyboxData& skybox_data);
    void free();

  private:
    void create_pipeline_layout();
    void create_descriptor_set();
    void create_pipeline_cache();
    void create_graphics_pipeline();
    void create_vertex_buffer();

    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptor_sets_;
    VkPipelineCache pipeline_cache_ = VK_NULL_HANDLE;
    VkShaderModule vertex_shader_ = VK_NULL_HANDLE;
    VkShaderModule fragment_shader_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkBuffer vertex_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertex_buffer_memory_ = VK_NULL_HANDLE;
  };
} // namespace gfx

#include "gfx/skybox-pipeline.hxx"
