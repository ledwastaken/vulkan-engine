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
    void create_uniform_buffer();

    VkShaderModule vertex_shader_ = VK_NULL_HANDLE;
    VkShaderModule fragment_shader_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkPipelineCache pipeline_cache_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
    VkBuffer uniform_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory uniform_buffer_memory_ = VK_NULL_HANDLE;
    void* uniform_buffer_data_;
  };
} // namespace render
