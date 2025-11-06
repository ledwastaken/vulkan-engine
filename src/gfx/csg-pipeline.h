#pragma once

#include <vector>

#include "gfx/pipeline.h"
#include "misc/singleton.h"
#include "scene/mesh.h"
#include "types/matrix4.h"

namespace gfx
{
  class CSGPipeline
    : public misc::Singleton<CSGPipeline>
    , public Pipeline
  {
    // Give Singleton access to class’s private constructor
    friend class Singleton<CSGPipeline>;

  private:
    CSGPipeline() = default;

  public:
    void init();
    void draw(VkImageView image_view, VkCommandBuffer command_buffer, const types::Matrix4& view,
              const types::Matrix4& projection, scene::Mesh& mesh);
    void free();

  private:
    void create_pipeline_layout();
    void create_descriptor_set();
    void create_pipeline_cache();
    void create_graphics_pipeline();
    void create_uniform_buffer();

    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptor_sets_;
    VkPipelineCache pipeline_cache_ = VK_NULL_HANDLE;
    VkShaderModule vertex_shader_ = VK_NULL_HANDLE;
    VkShaderModule fragment_shader_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    std::vector<VkBuffer> uniform_buffers_;
    std::vector<VkDeviceMemory> uniform_buffers_memory_;
    std::vector<void*> uniform_buffers_data_;
  };
} // namespace gfx
