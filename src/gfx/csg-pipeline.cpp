#include "gfx/csg-pipeline.h"

#include <cstring>
#include <string>

#include "core/engine.h"

namespace gfx
{
  void CSGPipeline::init()
  {
    create_pipeline_layout();
    create_descriptor_set();
    create_pipeline_cache();

    create_shader_module("csg.vert.spv", &vertex_shader_);
    create_shader_module("csg.frag.spv", &fragment_shader_);
    create_shader_module("depth.frag.spv", &depth_shader_);

    create_graphics_pipeline();
    create_uniform_buffer();
  }

  void CSGPipeline::draw(VkImageView image_view, VkImageView depth_view,
                         VkCommandBuffer command_buffer, const types::Matrix4& view,
                         const types::Matrix4& projection, scene::Mesh& mesh,
                         scene::Mesh& substractive_mesh)
  {
    auto& engine = core::Engine::get_singleton();
    auto extent = engine.get_swapchain_extent();

    std::memcpy(uniform_buffers_data_[engine.get_current_frame()], view.data(), 16 * sizeof(float));
    std::memcpy(uniform_buffers_data_[engine.get_current_frame()] + 64, projection.data(),
                16 * sizeof(float));

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 1,
                            &descriptor_sets_[engine.get_current_frame()], 0, nullptr);

    const VkViewport viewport{
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(extent.width),
      .height = static_cast<float>(extent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
    };

    const VkRect2D scissor = {
      .offset = { 0, 0 },
      .extent = extent,
    };

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkDeviceSize offset = 0;
    VkBuffer vertex_buffer;

    const VkClearValue clear_value = {};
    const VkRenderingAttachmentInfo color_attachment = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .pNext = nullptr,
      .imageView = image_view,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .resolveImageView = VK_NULL_HANDLE,
      .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = clear_value,
    };

    const VkClearValue depth_clear_value = { .depthStencil = { .depth = 1.0f, .stencil = 0 } };
    const VkRenderingAttachmentInfo depth_attachment = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .pNext = nullptr,
      .imageView = depth_view,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .resolveImageView = VK_NULL_HANDLE,
      .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = depth_clear_value,
    };

    const VkRect2D render_area = {
      .offset = { 0, 0 },
      .extent = extent,
    };

    const VkRenderingInfo rendering_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .pNext = nullptr,
      .flags = 0,
      .renderArea = render_area,
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment,
      .pDepthAttachment = &depth_attachment,
      .pStencilAttachment = &depth_attachment,
    };

    vkCmdSetStencilWriteMask(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0xFF);
    vkCmdSetStencilCompareMask(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0xFF);

    vkCmdBeginRendering(command_buffer, &rendering_info);

    static bool active = false;
    ImGui::Checkbox("Active", &active);

    static bool stencil = true;
    ImGui::Checkbox("Use stencil buffer", &stencil);

    if (stencil)
    {
      if (!active)
      {
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
                           mesh.cframe.to_matrix().data());

        int one = 1;
        vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 64, 4,
                           &one);

        vkCmdSetCullMode(command_buffer, VK_CULL_MODE_BACK_BIT);
        vkCmdSetDepthTestEnable(command_buffer, VK_TRUE);
        vkCmdSetDepthWriteEnable(command_buffer, VK_TRUE);
        vkCmdSetDepthCompareOp(command_buffer, VK_COMPARE_OP_LESS);
        vkCmdSetStencilTestEnable(command_buffer, VK_FALSE);

        vertex_buffer = mesh.get_vertex_buffer();
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
        vkCmdBindIndexBuffer(command_buffer, mesh.get_index_buffer(), offset, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffer, mesh.get_index_count(), 1, 0, 0, 0);

        vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
                           substractive_mesh.cframe.to_matrix().data());
        vertex_buffer = substractive_mesh.get_vertex_buffer();
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
        vkCmdBindIndexBuffer(command_buffer, substractive_mesh.get_index_buffer(), offset,
                             VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffer, substractive_mesh.get_index_count(), 1, 0, 0, 0);

        vkCmdEndRendering(command_buffer);
        return;
      }

      vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, depth_pipeline_);
      vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
                         mesh.cframe.to_matrix().data());

      int one = 1;
      vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 64, 4, &one);

      vkCmdSetCullMode(command_buffer, VK_CULL_MODE_BACK_BIT);
      vkCmdSetDepthTestEnable(command_buffer, VK_TRUE);
      vkCmdSetDepthWriteEnable(command_buffer, VK_TRUE);
      vkCmdSetDepthCompareOp(command_buffer, VK_COMPARE_OP_LESS);
      vkCmdSetStencilTestEnable(command_buffer, VK_FALSE);

      vertex_buffer = mesh.get_vertex_buffer();
      vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      vkCmdBindIndexBuffer(command_buffer, mesh.get_index_buffer(), offset, VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(command_buffer, mesh.get_index_count(), 1, 0, 0, 0);

      vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, depth_pipeline_);
      vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
                         substractive_mesh.cframe.to_matrix().data());

      vkCmdSetCullMode(command_buffer, VK_CULL_MODE_FRONT_BIT);
      vkCmdSetDepthTestEnable(command_buffer, VK_TRUE);
      vkCmdSetDepthWriteEnable(command_buffer, VK_FALSE);
      vkCmdSetDepthCompareOp(command_buffer, VK_COMPARE_OP_LESS);
      vkCmdSetStencilTestEnable(command_buffer, VK_TRUE);
      vkCmdSetStencilOp(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP,
                        VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE, VK_COMPARE_OP_ALWAYS);
      vkCmdSetStencilReference(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 1);

      vertex_buffer = substractive_mesh.get_vertex_buffer();
      vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      vkCmdBindIndexBuffer(command_buffer, substractive_mesh.get_index_buffer(), offset,
                           VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(command_buffer, substractive_mesh.get_index_count(), 1, 0, 0, 0);

      vkCmdSetCullMode(command_buffer, VK_CULL_MODE_BACK_BIT);
      vkCmdSetDepthTestEnable(command_buffer, VK_TRUE);
      vkCmdSetDepthWriteEnable(command_buffer, VK_FALSE);
      vkCmdSetDepthCompareOp(command_buffer, VK_COMPARE_OP_LESS);
      vkCmdSetStencilTestEnable(command_buffer, VK_TRUE);
      vkCmdSetStencilOp(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP,
                        VK_STENCIL_OP_KEEP, VK_STENCIL_OP_ZERO, VK_COMPARE_OP_ALWAYS);

      vkCmdDrawIndexed(command_buffer, substractive_mesh.get_index_count(), 1, 0, 0, 0);

      vkCmdEndRendering(command_buffer);

      const VkRenderingAttachmentInfo stencil_attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = depth_view,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = depth_clear_value,
      };

      const VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea = render_area,
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = &depth_attachment,
        .pStencilAttachment = &stencil_attachment,
      };

      vkCmdBeginRendering(command_buffer, &rendering_info);

      vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, depth_pipeline_);

      vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
                         mesh.cframe.to_matrix().data());

      vkCmdSetCullMode(command_buffer, VK_CULL_MODE_FRONT_BIT);
      vkCmdSetDepthTestEnable(command_buffer, VK_TRUE);
      vkCmdSetDepthWriteEnable(command_buffer, VK_TRUE);
      vkCmdSetStencilTestEnable(command_buffer, VK_TRUE);
      vkCmdSetStencilOp(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP,
                        VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL);
      vkCmdSetStencilReference(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 1);

      vertex_buffer = mesh.get_vertex_buffer();
      vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      vkCmdBindIndexBuffer(command_buffer, mesh.get_index_buffer(), offset, VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(command_buffer, mesh.get_index_count(), 1, 0, 0, 0);

      vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
                         substractive_mesh.cframe.to_matrix().data());

      vkCmdSetCullMode(command_buffer, VK_CULL_MODE_FRONT_BIT);
      vkCmdSetDepthTestEnable(command_buffer, VK_TRUE);
      vkCmdSetDepthWriteEnable(command_buffer, VK_FALSE);
      vkCmdSetStencilTestEnable(command_buffer, VK_TRUE);
      vkCmdSetStencilOp(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP,
                        VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE, VK_COMPARE_OP_ALWAYS);
      vkCmdSetStencilReference(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 42);

      vertex_buffer = substractive_mesh.get_vertex_buffer();
      vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      vkCmdBindIndexBuffer(command_buffer, substractive_mesh.get_index_buffer(), offset,
                           VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(command_buffer, substractive_mesh.get_index_count(), 1, 0, 0, 0);

      vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
      vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
                         mesh.cframe.to_matrix().data());

      vkCmdSetCullMode(command_buffer, VK_CULL_MODE_BACK_BIT);
      vkCmdSetDepthTestEnable(command_buffer, VK_FALSE);
      vkCmdSetDepthWriteEnable(command_buffer, VK_FALSE);
      vkCmdSetStencilTestEnable(command_buffer, VK_TRUE);
      vkCmdSetStencilOp(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP,
                        VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL);
      vkCmdSetStencilReference(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0);

      vertex_buffer = mesh.get_vertex_buffer();
      vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      vkCmdBindIndexBuffer(command_buffer, mesh.get_index_buffer(), offset, VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(command_buffer, mesh.get_index_count(), 1, 0, 0, 0);

      vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
                         substractive_mesh.cframe.to_matrix().data());

      int minus_one = -1;
      vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 64, 4,
                         &minus_one);

      vkCmdSetCullMode(command_buffer, VK_CULL_MODE_FRONT_BIT);
      vkCmdSetDepthTestEnable(command_buffer, VK_FALSE);
      vkCmdSetDepthWriteEnable(command_buffer, VK_FALSE);
      vkCmdSetStencilTestEnable(command_buffer, VK_TRUE);
      vkCmdSetStencilOp(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP,
                        VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL);
      vkCmdSetStencilReference(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 1);

      vertex_buffer = substractive_mesh.get_vertex_buffer();
      vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      vkCmdBindIndexBuffer(command_buffer, substractive_mesh.get_index_buffer(), offset,
                           VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(command_buffer, substractive_mesh.get_index_count(), 1, 0, 0, 0);
    }

    vkCmdEndRendering(command_buffer);
  }

  void CSGPipeline::free()
  {
    auto& engine = core::Engine::get_singleton();

    vkDestroyPipeline(engine.get_device(), pipeline_, nullptr);
    vkDestroyPipeline(engine.get_device(), depth_pipeline_, nullptr);

    vkDestroyShaderModule(engine.get_device(), fragment_shader_, nullptr);
    vkDestroyShaderModule(engine.get_device(), vertex_shader_, nullptr);
    vkDestroyShaderModule(engine.get_device(), depth_shader_, nullptr);

    vkDestroyPipelineCache(engine.get_device(), pipeline_cache_, nullptr);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
      vkUnmapMemory(engine.get_device(), uniform_buffers_memory_[i]);
      vkDestroyBuffer(engine.get_device(), uniform_buffers_[i], nullptr);
      vkFreeMemory(engine.get_device(), uniform_buffers_memory_[i], nullptr);
    }

    vkFreeDescriptorSets(engine.get_device(), descriptor_pool_, 1, descriptor_sets_.data());
    vkDestroyDescriptorPool(engine.get_device(), descriptor_pool_, nullptr);
    vkDestroyPipelineLayout(engine.get_device(), pipeline_layout_, nullptr);
    vkDestroyDescriptorSetLayout(engine.get_device(), descriptor_set_layout_, nullptr);
  }

  void CSGPipeline::create_pipeline_layout()
  {
    auto& engine = core::Engine::get_singleton();

    const VkDescriptorSetLayoutBinding layout_binding = {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .pImmutableSamplers = nullptr,
    };

    const VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .bindingCount = 1,
      .pBindings = &layout_binding,
    };

    VkResult result = vkCreateDescriptorSetLayout(
        engine.get_device(), &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor set layout");

    const VkPushConstantRange push_constant_ranges[] = {
      {
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
          .offset = 0,
          .size = 68,
      },
    };

    const VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptor_set_layout_,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = push_constant_ranges,
    };

    result = vkCreatePipelineLayout(engine.get_device(), &pipeline_layout_create_info, nullptr,
                                    &pipeline_layout_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create pipeline layout");
  }

  void CSGPipeline::create_descriptor_set()
  {
    auto& engine = core::Engine::get_singleton();

    const VkDescriptorPoolSize pool_size = {
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = MAX_FRAMES_IN_FLIGHT,
    };

    const VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = MAX_FRAMES_IN_FLIGHT,
      .poolSizeCount = 1,
      .pPoolSizes = &pool_size,
    };

    VkResult result = vkCreateDescriptorPool(engine.get_device(), &descriptor_pool_create_info,
                                             nullptr, &descriptor_pool_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor pool");

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptor_set_layout_);

    const VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = descriptor_pool_,
      .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
      .pSetLayouts = layouts.data(),
    };

    descriptor_sets_.resize(MAX_FRAMES_IN_FLIGHT);
    result = vkAllocateDescriptorSets(engine.get_device(), &descriptor_set_allocate_info,
                                      descriptor_sets_.data());
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to allocate descriptor set");
  }

  void CSGPipeline::create_pipeline_cache()
  {
    auto& engine = core::Engine::get_singleton();
    auto device = engine.get_device();

    const VkPipelineCacheCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .initialDataSize = 0,
      .pInitialData = nullptr,
    };

    VkResult result = vkCreatePipelineCache(device, &create_info, nullptr, &pipeline_cache_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create pipeline cache");
  }

  void CSGPipeline::create_graphics_pipeline()
  {
    auto& engine = core::Engine::get_singleton();
    auto device = engine.get_device();

    const VkPipelineShaderStageCreateInfo shader_stage_infos[] = {
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vertex_shader_,
          .pName = "main",
          .pSpecializationInfo = nullptr,
      },
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = fragment_shader_,
          .pName = "main",
          .pSpecializationInfo = nullptr,
      },
    };

    const VkVertexInputBindingDescription vertex_binding_description = {
      .binding = 0,
      .stride = 32,
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    const VkVertexInputAttributeDescription vertex_attribute_descriptions[] = {
      {
          .location = 0,
          .binding = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 0,
      },
      {
          .location = 1,
          .binding = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 12,
      },
      {
          .location = 2,
          .binding = 0,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .offset = 24,
      },
    };

    const VkPipelineVertexInputStateCreateInfo vertex_input_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &vertex_binding_description,
      .vertexAttributeDescriptionCount = 3,
      .pVertexAttributeDescriptions = vertex_attribute_descriptions,
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
    };

    const VkPipelineRasterizationStateCreateInfo rasterization_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .depthBiasConstantFactor = 0.0f,
      .depthBiasClamp = 0.0f,
      .depthBiasSlopeFactor = 0.0f,
      .lineWidth = 1.0f,
    };

    const VkPipelineMultisampleStateCreateInfo multisample_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 0.0f,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE,
    };

    const VkPipelineColorBlendAttachmentState color_blend_attachment = {
      .blendEnable = VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    const VkPipelineColorBlendStateCreateInfo color_blend_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = 1,
      .pAttachments = &color_blend_attachment,
      .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
    };

    const VkDynamicState dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
      VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
      VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
      VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
      VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE,
      VK_DYNAMIC_STATE_STENCIL_OP,
      VK_DYNAMIC_STATE_STENCIL_REFERENCE,
      VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
      VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
      VK_DYNAMIC_STATE_CULL_MODE,
    };

    const VkPipelineDynamicStateCreateInfo dynamic_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .dynamicStateCount = sizeof(dynamic_states) / sizeof(dynamic_states[0]),
      .pDynamicStates = dynamic_states,
    };

    const VkPipelineViewportStateCreateInfo viewport_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .viewportCount = 1,
      .pViewports = nullptr,
      .scissorCount = 1,
      .pScissors = nullptr,
    };

    const VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
      .front = {},
      .back = {},
      .minDepthBounds = 0.0f,
      .maxDepthBounds = 1.0f,
    };

    const VkFormat color_attachments[] = { engine.get_surface_format().format };

    const VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .pNext = nullptr,
      .viewMask = 0,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = color_attachments,
      .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT,
      .stencilAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT,
    };

    const VkGraphicsPipelineCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &pipeline_rendering_create_info,
      .flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT,
      .stageCount = 2,
      .pStages = shader_stage_infos,
      .pVertexInputState = &vertex_input_state,
      .pInputAssemblyState = &input_assembly_state,
      .pTessellationState = nullptr,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterization_state,
      .pMultisampleState = &multisample_state,
      .pDepthStencilState = &depth_stencil_state,
      .pColorBlendState = &color_blend_state,
      .pDynamicState = &dynamic_state,
      .layout = pipeline_layout_,
      .renderPass = VK_NULL_HANDLE,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0,
    };

    VkResult result =
        vkCreateGraphicsPipelines(device, pipeline_cache_, 1, &create_info, nullptr, &pipeline_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create graphics pipeline");

    const VkPipelineShaderStageCreateInfo depth_shader_stage_infos[] = {
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vertex_shader_,
          .pName = "main",
          .pSpecializationInfo = nullptr,
      },
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = depth_shader_,
          .pName = "main",
          .pSpecializationInfo = nullptr,
      },
    };

    const VkGraphicsPipelineCreateInfo depth_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &pipeline_rendering_create_info,
      .flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT,
      .stageCount = 2,
      .pStages = depth_shader_stage_infos,
      .pVertexInputState = &vertex_input_state,
      .pInputAssemblyState = &input_assembly_state,
      .pTessellationState = nullptr,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterization_state,
      .pMultisampleState = &multisample_state,
      .pDepthStencilState = &depth_stencil_state,
      .pColorBlendState = &color_blend_state,
      .pDynamicState = &dynamic_state,
      .layout = pipeline_layout_,
      .renderPass = VK_NULL_HANDLE,
      .subpass = 0,
      .basePipelineHandle = pipeline_,
      .basePipelineIndex = -1,
    };

    result = vkCreateGraphicsPipelines(device, pipeline_cache_, 1, &depth_pipeline_info, nullptr,
                                       &depth_pipeline_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create graphics pipeline");
  }

  void CSGPipeline::create_uniform_buffer()
  {
    auto& engine = core::Engine::get_singleton();
    VkDeviceSize buffer_size = 128;

    uniform_buffers_.resize(MAX_FRAMES_IN_FLIGHT);
    uniform_buffers_memory_.resize(MAX_FRAMES_IN_FLIGHT);
    uniform_buffers_data_.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
      const VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = buffer_size,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
      };

      engine.create_buffer(buffer_create_info,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                               | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           uniform_buffers_[i], uniform_buffers_memory_[i]);

      VkDeviceSize offset = 0;
      vkMapMemory(engine.get_device(), uniform_buffers_memory_[i], offset, buffer_size, 0,
                  &uniform_buffers_data_[i]);

      const VkDescriptorBufferInfo descriptor_buffer_info = {
        .buffer = uniform_buffers_[i],
        .offset = 0,
        .range = buffer_size,
      };

      const VkWriteDescriptorSet write_descriptor_set = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = descriptor_sets_[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &descriptor_buffer_info,
        .pTexelBufferView = nullptr,
      };

      vkUpdateDescriptorSets(engine.get_device(), 1, &write_descriptor_set, 0, nullptr);
    }
  }
} // namespace gfx
