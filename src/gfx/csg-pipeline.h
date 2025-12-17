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
    void draw(VkImageView image_view, VkImageView depth_view, VkCommandBuffer command_buffer,
              const types::Matrix4& view, const types::Matrix4& projection, scene::Mesh& mesh,
              scene::Mesh& substractive_mesh);
    void free();

  private:
    void create_pipeline_layout();
    void create_descriptor_set();
    void create_pipeline_cache();
    void create_graphics_pipeline();
    void create_uniform_buffer();
    void create_depth_image(VkImage& image, VkImageView& image_view, VkSampler& sampler,
                            VkDeviceMemory& memory);
    void bind_depth_images();

    void render_depth(VkCommandBuffer& command_buffer, scene::Mesh& mesh,
                      scene::Mesh& substractive_mesh);

    VkDescriptorSetLayout ubo_descriptor_set_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout textures_descriptor_set_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> ubo_descriptor_sets_;
    std::vector<VkDescriptorSet> textures_descriptor_sets_;
    VkPipelineCache pipeline_cache_ = VK_NULL_HANDLE;
    VkShaderModule vertex_shader_ = VK_NULL_HANDLE;
    VkShaderModule fragment_shader_ = VK_NULL_HANDLE;
    VkShaderModule depth_shader_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipeline depth_pipeline_ = VK_NULL_HANDLE;
    std::vector<VkBuffer> uniform_buffers_;
    std::vector<VkDeviceMemory> uniform_buffers_memory_;
    std::vector<void*> uniform_buffers_data_;

    VkImage ray_enter_image_ = VK_NULL_HANDLE;
    VkDeviceMemory ray_enter_memory_ = VK_NULL_HANDLE;
    VkImageView ray_enter_view_ = VK_NULL_HANDLE;
    VkSampler ray_enter_sampler_ = VK_NULL_HANDLE;

    VkImage ray_leave_image_ = VK_NULL_HANDLE;
    VkDeviceMemory ray_leave_memory_ = VK_NULL_HANDLE;
    VkImageView ray_leave_view_ = VK_NULL_HANDLE;
    VkSampler ray_leave_sampler_ = VK_NULL_HANDLE;

    VkImage back_depth_image_ = VK_NULL_HANDLE;
    VkDeviceMemory back_depth_memory_ = VK_NULL_HANDLE;
    VkImageView back_depth_view_ = VK_NULL_HANDLE;
    VkSampler back_depth_sampler_ = VK_NULL_HANDLE;

    VkImage mask_image_ = VK_NULL_HANDLE;
    VkDeviceMemory mask_memory_ = VK_NULL_HANDLE;
    VkImageView mask_view_ = VK_NULL_HANDLE;
    VkSampler mask_sampler_ = VK_NULL_HANDLE;
  };
} // namespace gfx
