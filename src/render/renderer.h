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

    void init();
    void free();

    void operator()(scene::Scene& scene, uint32_t image_index);
    void operator()(scene::Mesh& mesh) override;

  private:
    void create_shader_module(const char* path, VkShaderModule* shader_module);
    void create_descriptor_set_layout();
    void create_pipeline_layout();
    void create_pipeline_cache();
    void create_pipeline();

    VkShaderModule vertex_shader_ = VK_NULL_HANDLE;
    VkShaderModule fragment_shader_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkPipelineCache pipeline_cache_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
  };
} // namespace render
