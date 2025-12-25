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
    create_shader_module("csg-diff-frontface.frag.spv", &frontface_shader_);
    create_shader_module("csg-diff.vert.spv", &vertex_frontface_shader_);

    create_graphics_pipeline();
    create_uniform_buffer();
    create_depth_image(ray_enter_image_, ray_enter_view_, ray_enter_sampler_, ray_enter_memory_);
    create_depth_image(ray_leave_image_, ray_leave_view_, ray_leave_sampler_, ray_leave_memory_);
    create_depth_image(back_depth_image_, back_depth_view_, back_depth_sampler_,
                       back_depth_memory_);

    auto& engine = core::Engine::get_singleton();

    const VkExtent3D image_extent = {
      .width = 800,
      .height = 600,
      .depth = 1,
    };

    const VkImageCreateInfo mask_image_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8_UNORM,
      .extent = image_extent,
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
          | VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    engine.create_image(mask_image_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mask_image_,
                        mask_memory_);

    const VkComponentMapping components = {
      .r = VK_COMPONENT_SWIZZLE_R,
      .g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    const VkImageSubresourceRange subresource_range = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    };

    const VkImageViewCreateInfo view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = mask_image_,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8_UNORM,
      .components = components,
      .subresourceRange = subresource_range,
    };

    VkResult result = vkCreateImageView(engine.get_device(), &view_info, nullptr, &mask_view_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create image view");

    const VkSamplerCreateInfo sampler_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .magFilter = VK_FILTER_NEAREST,
      .minFilter = VK_FILTER_NEAREST,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 0.0f,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_NEVER,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
    };

    result = vkCreateSampler(engine.get_device(), &sampler_info, nullptr, &mask_sampler_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create sampler");

    const core::TransitionLayout transition_layout = {
      .src_access = 0,
      .dst_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      .src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      .dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
      .old_layout = VK_IMAGE_LAYOUT_UNDEFINED,
      .new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    engine.transition_image_layout(mask_image_, VK_FORMAT_R8_UNORM, 1, transition_layout);

    bind_depth_images();
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

    // TODO: Update and bind textures descriptor sets

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 1,
                            &ubo_descriptor_sets_[engine.get_current_frame()], 0, nullptr);

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

    const VkClearValue clear_value = {};
    const VkRenderingAttachmentInfo attachments[] = {
      {
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
      },
      {
          .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
          .pNext = nullptr,
          .imageView = mask_view_,
          .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .resolveMode = VK_RESOLVE_MODE_NONE,
          .resolveImageView = VK_NULL_HANDLE,
          .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .clearValue = clear_value,
      },
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
      .colorAttachmentCount = 2,
      .pColorAttachments = attachments,
      .pDepthAttachment = &depth_attachment,
      .pStencilAttachment = &depth_attachment,
    };

    vkCmdSetStencilWriteMask(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0xFF);
    vkCmdSetStencilCompareMask(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0xFF);

    static bool active = false;
    ImGui::Checkbox("Active", &active);

    static bool stencil = false;
    ImGui::Checkbox("Use stencil buffer", &stencil);

    if (stencil)
    {
      // vkCmdBeginRendering(command_buffer, &rendering_info);

      // if (!active)
      // {
      //   vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
      //   vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
      //                      mesh.cframe.to_matrix().data());

      //   int one = 1;
      //   vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 64, 4,
      //                      &one);

      //   vkCmdSetCullMode(command_buffer, VK_CULL_MODE_BACK_BIT);
      //   vkCmdSetDepthTestEnable(command_buffer, VK_TRUE);
      //   vkCmdSetDepthWriteEnable(command_buffer, VK_TRUE);
      //   vkCmdSetDepthCompareOp(command_buffer, VK_COMPARE_OP_LESS);
      //   vkCmdSetStencilTestEnable(command_buffer, VK_FALSE);

      //   VkDeviceSize offset = 0;
      //   VkBuffer vertex_buffer = mesh.get_vertex_buffer();
      //   vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      //   vkCmdBindIndexBuffer(command_buffer, mesh.get_index_buffer(), offset,
      //   VK_INDEX_TYPE_UINT32); vkCmdDrawIndexed(command_buffer, mesh.get_index_count(), 1, 0, 0,
      //   0);

      //   vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
      //                      substractive_mesh.cframe.to_matrix().data());
      //   vertex_buffer = substractive_mesh.get_vertex_buffer();
      //   vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      //   vkCmdBindIndexBuffer(command_buffer, substractive_mesh.get_index_buffer(), offset,
      //                        VK_INDEX_TYPE_UINT32);
      //   vkCmdDrawIndexed(command_buffer, substractive_mesh.get_index_count(), 1, 0, 0, 0);
      // }

      // vkCmdEndRendering(command_buffer);

      // vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, depth_pipeline_);
      // vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
      //                    mesh.cframe.to_matrix().data());

      // int one = 1;
      // vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 64, 4,
      // &one);

      // vkCmdSetCullMode(command_buffer, VK_CULL_MODE_BACK_BIT);
      // vkCmdSetDepthTestEnable(command_buffer, VK_TRUE);
      // vkCmdSetDepthWriteEnable(command_buffer, VK_TRUE);
      // vkCmdSetDepthCompareOp(command_buffer, VK_COMPARE_OP_LESS);
      // vkCmdSetStencilTestEnable(command_buffer, VK_FALSE);

      // vertex_buffer = mesh.get_vertex_buffer();
      // vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      // vkCmdBindIndexBuffer(command_buffer, mesh.get_index_buffer(), offset,
      // VK_INDEX_TYPE_UINT32); vkCmdDrawIndexed(command_buffer, mesh.get_index_count(), 1, 0, 0,
      // 0);

      // vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, depth_pipeline_);
      // vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
      //                    substractive_mesh.cframe.to_matrix().data());

      // vkCmdSetCullMode(command_buffer, VK_CULL_MODE_FRONT_BIT);
      // vkCmdSetDepthTestEnable(command_buffer, VK_TRUE);
      // vkCmdSetDepthWriteEnable(command_buffer, VK_FALSE);
      // vkCmdSetDepthCompareOp(command_buffer, VK_COMPARE_OP_LESS);
      // vkCmdSetStencilTestEnable(command_buffer, VK_TRUE);
      // vkCmdSetStencilOp(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP,
      //                   VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE, VK_COMPARE_OP_ALWAYS);
      // vkCmdSetStencilReference(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 1);

      // vertex_buffer = substractive_mesh.get_vertex_buffer();
      // vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      // vkCmdBindIndexBuffer(command_buffer, substractive_mesh.get_index_buffer(), offset,
      //                      VK_INDEX_TYPE_UINT32);
      // vkCmdDrawIndexed(command_buffer, substractive_mesh.get_index_count(), 1, 0, 0, 0);

      // vkCmdSetCullMode(command_buffer, VK_CULL_MODE_BACK_BIT);
      // vkCmdSetDepthTestEnable(command_buffer, VK_TRUE);
      // vkCmdSetDepthWriteEnable(command_buffer, VK_FALSE);
      // vkCmdSetDepthCompareOp(command_buffer, VK_COMPARE_OP_LESS);
      // vkCmdSetStencilTestEnable(command_buffer, VK_TRUE);
      // vkCmdSetStencilOp(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP,
      //                   VK_STENCIL_OP_KEEP, VK_STENCIL_OP_ZERO, VK_COMPARE_OP_ALWAYS);

      // vkCmdDrawIndexed(command_buffer, substractive_mesh.get_index_count(), 1, 0, 0, 0);

      // vkCmdEndRendering(command_buffer);

      // const VkRenderingAttachmentInfo stencil_attachment = {
      //   .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      //   .pNext = nullptr,
      //   .imageView = depth_view,
      //   .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      //   .resolveMode = VK_RESOLVE_MODE_NONE,
      //   .resolveImageView = VK_NULL_HANDLE,
      //   .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      //   .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
      //   .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      //   .clearValue = depth_clear_value,
      // };

      // const VkRenderingInfo rendering_info = {
      //   .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      //   .pNext = nullptr,
      //   .flags = 0,
      //   .renderArea = render_area,
      //   .layerCount = 1,
      //   .viewMask = 0,
      //   .colorAttachmentCount = 1,
      //   .pColorAttachments = &color_attachment,
      //   .pDepthAttachment = &depth_attachment,
      //   .pStencilAttachment = &stencil_attachment,
      // };

      // vkCmdBeginRendering(command_buffer, &rendering_info);

      // vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, depth_pipeline_);

      // vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
      //                    mesh.cframe.to_matrix().data());

      // vkCmdSetCullMode(command_buffer, VK_CULL_MODE_FRONT_BIT);
      // vkCmdSetDepthTestEnable(command_buffer, VK_TRUE);
      // vkCmdSetDepthWriteEnable(command_buffer, VK_TRUE);
      // vkCmdSetStencilTestEnable(command_buffer, VK_TRUE);
      // vkCmdSetStencilOp(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP,
      //                   VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL);
      // vkCmdSetStencilReference(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 1);

      // vertex_buffer = mesh.get_vertex_buffer();
      // vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      // vkCmdBindIndexBuffer(command_buffer, mesh.get_index_buffer(), offset,
      // VK_INDEX_TYPE_UINT32); vkCmdDrawIndexed(command_buffer, mesh.get_index_count(), 1, 0, 0,
      // 0);

      // vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
      //                    substractive_mesh.cframe.to_matrix().data());

      // vkCmdSetCullMode(command_buffer, VK_CULL_MODE_FRONT_BIT);
      // vkCmdSetDepthTestEnable(command_buffer, VK_TRUE);
      // vkCmdSetDepthWriteEnable(command_buffer, VK_FALSE);
      // vkCmdSetStencilTestEnable(command_buffer, VK_TRUE);
      // vkCmdSetStencilOp(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP,
      //                   VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE, VK_COMPARE_OP_ALWAYS);
      // vkCmdSetStencilReference(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 42);

      // vertex_buffer = substractive_mesh.get_vertex_buffer();
      // vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      // vkCmdBindIndexBuffer(command_buffer, substractive_mesh.get_index_buffer(), offset,
      //                      VK_INDEX_TYPE_UINT32);
      // vkCmdDrawIndexed(command_buffer, substractive_mesh.get_index_count(), 1, 0, 0, 0);

      // vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
      // vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
      //                    mesh.cframe.to_matrix().data());

      // vkCmdSetCullMode(command_buffer, VK_CULL_MODE_BACK_BIT);
      // vkCmdSetDepthTestEnable(command_buffer, VK_FALSE);
      // vkCmdSetDepthWriteEnable(command_buffer, VK_FALSE);
      // vkCmdSetStencilTestEnable(command_buffer, VK_TRUE);
      // vkCmdSetStencilOp(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP,
      //                   VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL);
      // vkCmdSetStencilReference(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0);

      // vertex_buffer = mesh.get_vertex_buffer();
      // vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      // vkCmdBindIndexBuffer(command_buffer, mesh.get_index_buffer(), offset,
      // VK_INDEX_TYPE_UINT32); vkCmdDrawIndexed(command_buffer, mesh.get_index_count(), 1, 0, 0,
      // 0);

      // vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
      //                    substractive_mesh.cframe.to_matrix().data());

      // int minus_one = -1;
      // vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 64, 4,
      //                    &minus_one);

      // vkCmdSetCullMode(command_buffer, VK_CULL_MODE_FRONT_BIT);
      // vkCmdSetDepthTestEnable(command_buffer, VK_FALSE);
      // vkCmdSetDepthWriteEnable(command_buffer, VK_FALSE);
      // vkCmdSetStencilTestEnable(command_buffer, VK_TRUE);
      // vkCmdSetStencilOp(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP,
      //                   VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL);
      // vkCmdSetStencilReference(command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK, 1);

      // vertex_buffer = substractive_mesh.get_vertex_buffer();
      // vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      // vkCmdBindIndexBuffer(command_buffer, substractive_mesh.get_index_buffer(), offset,
      //                      VK_INDEX_TYPE_UINT32);
      // vkCmdDrawIndexed(command_buffer, substractive_mesh.get_index_count(), 1, 0, 0, 0);
    }
    else
    {
      render_depth(command_buffer, mesh, substractive_mesh);

      const VkImageSubresourceRange subresource_range = {
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      };

      const VkImageMemoryBarrier ray_enter_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = ray_enter_image_,
        .subresourceRange = subresource_range,
      };

      const VkImageMemoryBarrier ray_leave_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = ray_leave_image_,
        .subresourceRange = subresource_range,
      };

      const VkImageMemoryBarrier back_depth_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = back_depth_image_,
        .subresourceRange = subresource_range,
      };

      vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                           &ray_enter_memory_barrier);
      vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                           &ray_leave_memory_barrier);
      vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                           &back_depth_memory_barrier);

      vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
      vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 1,
                              1, &textures_descriptor_sets_[engine.get_current_frame()], 0,
                              nullptr);

      vkCmdBeginRendering(command_buffer, &rendering_info);

      vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
                         substractive_mesh.cframe.to_matrix().data());

      int minus_one = -1;
      vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 64, 4,
                         &minus_one);

      vkCmdSetCullMode(command_buffer, VK_CULL_MODE_FRONT_BIT);

      VkDeviceSize offset = 0;
      VkBuffer vertex_buffer = substractive_mesh.get_vertex_buffer();
      vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      vkCmdBindIndexBuffer(command_buffer, substractive_mesh.get_index_buffer(), offset,
                           VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(command_buffer, substractive_mesh.get_index_count(), 1, 0, 0, 0);

      const VkImageMemoryBarrier ray_enter_memory_barrier2 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = ray_enter_image_,
        .subresourceRange = subresource_range,
      };

      const VkImageMemoryBarrier ray_leave_memory_barrier2 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = ray_leave_image_,
        .subresourceRange = subresource_range,
      };

      const VkImageMemoryBarrier back_depth_memory_barrier2 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = back_depth_image_,
        .subresourceRange = subresource_range,
      };

      vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                           &ray_enter_memory_barrier2);
      vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                           &ray_leave_memory_barrier2);
      vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                           &back_depth_memory_barrier2);

      // Render front
      const VkImageSubresourceRange mask_subresource_range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      };

      const VkImageMemoryBarrier mask_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = mask_image_,
        .subresourceRange = mask_subresource_range,
      };

      vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                           &mask_memory_barrier);

      vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, frontface_pipeline_);

      vkCmdPushConstants(command_buffer, frontface_pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0,
                         64, mesh.cframe.to_matrix().data());

      vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              frontface_pipeline_layout_, 0, 1,
                              &ubo_descriptor_sets_[engine.get_current_frame()], 0, nullptr);

      vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              frontface_pipeline_layout_, 1, 1,
                              &frontface_descriptor_sets_[engine.get_current_frame()], 0, nullptr);
      vkCmdSetCullMode(command_buffer, VK_CULL_MODE_BACK_BIT);

      vertex_buffer = mesh.get_vertex_buffer();
      vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
      vkCmdBindIndexBuffer(command_buffer, mesh.get_index_buffer(), offset, VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(command_buffer, mesh.get_index_count(), 1, 0, 0, 0);

      const VkImageMemoryBarrier mask_memory_barrier2 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = mask_image_,
        .subresourceRange = mask_subresource_range,
      };

      vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr,
                           1, &mask_memory_barrier2);

      vkCmdEndRendering(command_buffer);
    }
  }

  void CSGPipeline::free()
  {
    auto& engine = core::Engine::get_singleton();

    vkDestroyPipeline(engine.get_device(), pipeline_, nullptr);
    vkDestroyPipeline(engine.get_device(), depth_pipeline_, nullptr);
    vkDestroyPipeline(engine.get_device(), frontface_pipeline_, nullptr);

    vkDestroyShaderModule(engine.get_device(), fragment_shader_, nullptr);
    vkDestroyShaderModule(engine.get_device(), vertex_shader_, nullptr);
    vkDestroyShaderModule(engine.get_device(), depth_shader_, nullptr);
    vkDestroyShaderModule(engine.get_device(), frontface_shader_, nullptr);
    vkDestroyShaderModule(engine.get_device(), vertex_frontface_shader_, nullptr);

    vkDestroyPipelineCache(engine.get_device(), pipeline_cache_, nullptr);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
      vkUnmapMemory(engine.get_device(), uniform_buffers_memory_[i]);
      vkDestroyBuffer(engine.get_device(), uniform_buffers_[i], nullptr);
      vkFreeMemory(engine.get_device(), uniform_buffers_memory_[i], nullptr);
    }

    vkFreeDescriptorSets(engine.get_device(), descriptor_pool_, 1, ubo_descriptor_sets_.data());
    vkFreeDescriptorSets(engine.get_device(), descriptor_pool_, 1,
                         textures_descriptor_sets_.data());
    vkFreeDescriptorSets(engine.get_device(), descriptor_pool_, 1,
                         frontface_descriptor_sets_.data());
    vkDestroyDescriptorPool(engine.get_device(), descriptor_pool_, nullptr);
    vkDestroyPipelineLayout(engine.get_device(), pipeline_layout_, nullptr);
    vkDestroyPipelineLayout(engine.get_device(), frontface_pipeline_layout_, nullptr);
    vkDestroyDescriptorSetLayout(engine.get_device(), ubo_descriptor_set_layout_, nullptr);
    vkDestroyDescriptorSetLayout(engine.get_device(), textures_descriptor_set_layout_, nullptr);
    vkDestroyDescriptorSetLayout(engine.get_device(), frontface_descriptor_set_layout_, nullptr);

    vkDestroySampler(engine.get_device(), ray_enter_sampler_, nullptr);
    vkDestroyImageView(engine.get_device(), ray_enter_view_, nullptr);
    vkDestroyImage(engine.get_device(), ray_enter_image_, nullptr);
    vkFreeMemory(engine.get_device(), ray_enter_memory_, nullptr);

    vkDestroySampler(engine.get_device(), ray_leave_sampler_, nullptr);
    vkDestroyImageView(engine.get_device(), ray_leave_view_, nullptr);
    vkDestroyImage(engine.get_device(), ray_leave_image_, nullptr);
    vkFreeMemory(engine.get_device(), ray_leave_memory_, nullptr);

    vkDestroySampler(engine.get_device(), back_depth_sampler_, nullptr);
    vkDestroyImageView(engine.get_device(), back_depth_view_, nullptr);
    vkDestroyImage(engine.get_device(), back_depth_image_, nullptr);
    vkFreeMemory(engine.get_device(), back_depth_memory_, nullptr);

    vkDestroySampler(engine.get_device(), mask_sampler_, nullptr);
    vkDestroyImageView(engine.get_device(), mask_view_, nullptr);
    vkDestroyImage(engine.get_device(), mask_image_, nullptr);
    vkFreeMemory(engine.get_device(), mask_memory_, nullptr);
  }

  void CSGPipeline::create_pipeline_layout()
  {
    auto& engine = core::Engine::get_singleton();

    const VkDescriptorSetLayoutBinding ubo_layout_binding = {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .pImmutableSamplers = nullptr,
    };

    const VkDescriptorSetLayoutCreateInfo ubo_descriptor_set_layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .bindingCount = 1,
      .pBindings = &ubo_layout_binding,
    };

    const VkDescriptorSetLayoutBinding texture_layout_bindings[] = {
      {
          .binding = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .pImmutableSamplers = nullptr,
      },
      {
          .binding = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .pImmutableSamplers = nullptr,
      },
      {
          .binding = 2,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .pImmutableSamplers = nullptr,
      },
    };

    const VkDescriptorSetLayoutCreateInfo textures_descriptor_set_layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .bindingCount = 3,
      .pBindings = texture_layout_bindings,
    };

    const VkDescriptorSetLayoutBinding frontface_bindings[] = {
      {
          .binding = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .pImmutableSamplers = nullptr,
      },
    };

    const VkDescriptorSetLayoutCreateInfo frontface_layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .bindingCount = 1,
      .pBindings = frontface_bindings,
    };

    VkResult result = vkCreateDescriptorSetLayout(
        engine.get_device(), &ubo_descriptor_set_layout_info, nullptr, &ubo_descriptor_set_layout_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor set layout");

    result = vkCreateDescriptorSetLayout(engine.get_device(), &textures_descriptor_set_layout_info,
                                         nullptr, &textures_descriptor_set_layout_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor set layout");

    result = vkCreateDescriptorSetLayout(engine.get_device(), &frontface_layout_info, nullptr,
                                         &frontface_descriptor_set_layout_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor set layout");

    const VkPushConstantRange push_constant_ranges[] = {
      {
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
          .offset = 0,
          .size = 68,
      },
    };

    std::vector<VkDescriptorSetLayout> descriptor_layouts = {
      ubo_descriptor_set_layout_,
      textures_descriptor_set_layout_,
    };

    const VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = 2,
      .pSetLayouts = descriptor_layouts.data(),
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = push_constant_ranges,
    };

    result = vkCreatePipelineLayout(engine.get_device(), &pipeline_layout_create_info, nullptr,
                                    &pipeline_layout_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create pipeline layout");

    const VkPushConstantRange frontface_push_constant_ranges[] = {
      {
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
          .offset = 0,
          .size = 64,
      },
    };

    std::vector<VkDescriptorSetLayout> frontface_descriptor_layouts = {
      ubo_descriptor_set_layout_,
      frontface_descriptor_set_layout_,
    };

    const VkPipelineLayoutCreateInfo frontface_pipeline_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = 2,
      .pSetLayouts = frontface_descriptor_layouts.data(),
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = frontface_push_constant_ranges,
    };

    result = vkCreatePipelineLayout(engine.get_device(), &frontface_pipeline_layout_create_info,
                                    nullptr, &frontface_pipeline_layout_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create pipeline layout");
  }

  void CSGPipeline::create_descriptor_set()
  {
    auto& engine = core::Engine::get_singleton();

    const VkDescriptorPoolSize pool_sizes[] = {
      {
          .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = MAX_FRAMES_IN_FLIGHT,
      },
      {
          .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = MAX_FRAMES_IN_FLIGHT * 4,
      },
    };

    const VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = MAX_FRAMES_IN_FLIGHT * 3,
      .poolSizeCount = 2,
      .pPoolSizes = pool_sizes,
    };

    VkResult result = vkCreateDescriptorPool(engine.get_device(), &descriptor_pool_create_info,
                                             nullptr, &descriptor_pool_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor pool");

    std::vector<VkDescriptorSetLayout> ubo_layouts(MAX_FRAMES_IN_FLIGHT,
                                                   ubo_descriptor_set_layout_);
    std::vector<VkDescriptorSetLayout> textures_layouts(MAX_FRAMES_IN_FLIGHT,
                                                        textures_descriptor_set_layout_);
    std::vector<VkDescriptorSetLayout> frontface_layouts(MAX_FRAMES_IN_FLIGHT,
                                                         frontface_descriptor_set_layout_);

    const VkDescriptorSetAllocateInfo ubo_descriptor_set_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = descriptor_pool_,
      .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
      .pSetLayouts = ubo_layouts.data(),
    };

    const VkDescriptorSetAllocateInfo textures_descriptor_set_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = descriptor_pool_,
      .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
      .pSetLayouts = textures_layouts.data(),
    };

    const VkDescriptorSetAllocateInfo frontface_descriptor_set_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = descriptor_pool_,
      .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
      .pSetLayouts = frontface_layouts.data(),
    };

    ubo_descriptor_sets_.resize(MAX_FRAMES_IN_FLIGHT);
    result = vkAllocateDescriptorSets(engine.get_device(), &ubo_descriptor_set_info,
                                      ubo_descriptor_sets_.data());
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to allocate descriptor set");

    textures_descriptor_sets_.resize(MAX_FRAMES_IN_FLIGHT);
    result = vkAllocateDescriptorSets(engine.get_device(), &textures_descriptor_set_info,
                                      textures_descriptor_sets_.data());
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to allocate descriptor set");

    frontface_descriptor_sets_.resize(MAX_FRAMES_IN_FLIGHT);
    result = vkAllocateDescriptorSets(engine.get_device(), &frontface_descriptor_set_info,
                                      frontface_descriptor_sets_.data());
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

    const VkPipelineColorBlendAttachmentState color_blend_attachments[] = {
      {
          .blendEnable = VK_TRUE,
          .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
          .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
          .colorBlendOp = VK_BLEND_OP_ADD,
          .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
          .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
          .alphaBlendOp = VK_BLEND_OP_ADD,
          .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
              | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      },
      {
          .blendEnable = VK_FALSE,
          .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
          .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
          .colorBlendOp = VK_BLEND_OP_ADD,
          .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
          .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
          .alphaBlendOp = VK_BLEND_OP_ADD,
          .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
              | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      },
    };

    const VkPipelineColorBlendStateCreateInfo color_blend_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = 2,
      .pAttachments = color_blend_attachments,
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

    const VkFormat color_attachments[] = {
      engine.get_surface_format().format,
      VK_FORMAT_R8_UNORM,
    };

    const VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .pNext = nullptr,
      .viewMask = 0,
      .colorAttachmentCount = 2,
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

    const VkPipelineRenderingCreateInfo depth_pipeline_rendering_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .pNext = nullptr,
      .viewMask = 0,
      .colorAttachmentCount = 0,
      .pColorAttachmentFormats = nullptr,
      .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
      .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    const VkGraphicsPipelineCreateInfo depth_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &depth_pipeline_rendering_info,
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

    const VkPipelineShaderStageCreateInfo frontface_shader_stage_infos[] = {
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vertex_frontface_shader_,
          .pName = "main",
          .pSpecializationInfo = nullptr,
      },
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = frontface_shader_,
          .pName = "main",
          .pSpecializationInfo = nullptr,
      },
    };

    VkFormat frontface_format = engine.get_surface_format().format;

    const VkPipelineRenderingCreateInfo frontface_pipeline_rendering_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .pNext = nullptr,
      .viewMask = 0,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &frontface_format,
      .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT,
      .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    const VkGraphicsPipelineCreateInfo frontface_create_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &pipeline_rendering_create_info,
      .flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT,
      .stageCount = 2,
      .pStages = frontface_shader_stage_infos,
      .pVertexInputState = &vertex_input_state,
      .pInputAssemblyState = &input_assembly_state,
      .pTessellationState = nullptr,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterization_state,
      .pMultisampleState = &multisample_state,
      .pDepthStencilState = &depth_stencil_state,
      .pColorBlendState = &color_blend_state,
      .pDynamicState = &dynamic_state,
      .layout = frontface_pipeline_layout_,
      .renderPass = VK_NULL_HANDLE,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0,
    };

    result = vkCreateGraphicsPipelines(device, pipeline_cache_, 1, &frontface_create_info, nullptr,
                                       &frontface_pipeline_);
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
        .dstSet = ubo_descriptor_sets_[i],
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

  void CSGPipeline::create_depth_image(VkImage& image, VkImageView& image_view, VkSampler& sampler,
                                       VkDeviceMemory& memory)
  {
    auto& engine = core::Engine::get_singleton();

    const VkExtent3D image_extent = {
      .width = 800,
      .height = 600,
      .depth = 1,
    };

    const VkImageCreateInfo depth_image_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_D32_SFLOAT,
      .extent = image_extent,
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
          | VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    engine.create_image(depth_image_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image,
                        memory);

    const VkComponentMapping depth_components = {
      .r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    const VkImageSubresourceRange depth_subresource_range = {
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    };

    const VkImageViewCreateInfo depth_view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_D32_SFLOAT,
      .components = depth_components,
      .subresourceRange = depth_subresource_range,
    };

    VkResult result =
        vkCreateImageView(engine.get_device(), &depth_view_info, nullptr, &image_view);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create image view");

    const VkSamplerCreateInfo sampler_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .magFilter = VK_FILTER_NEAREST,
      .minFilter = VK_FILTER_NEAREST,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 0.0f,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_NEVER,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
      .unnormalizedCoordinates = VK_FALSE,
    };

    result = vkCreateSampler(engine.get_device(), &sampler_info, nullptr, &sampler);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create sampler");

    const core::TransitionLayout transition_layout = {
      .src_access = 0,
      .dst_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      .src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      .dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .old_layout = VK_IMAGE_LAYOUT_UNDEFINED,
      .new_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
    };

    engine.transition_image_layout(image, VK_FORMAT_D32_SFLOAT, 1, transition_layout);
  }

  void CSGPipeline::bind_depth_images()
  {
    auto& engine = core::Engine::get_singleton();

    const VkDescriptorImageInfo ray_enter_image_info = {
      .sampler = ray_enter_sampler_,
      .imageView = ray_enter_view_,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    };

    const VkDescriptorImageInfo ray_leave_image_info = {
      .sampler = ray_leave_sampler_,
      .imageView = ray_leave_view_,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    };

    const VkDescriptorImageInfo back_depth_image_info = {
      .sampler = back_depth_sampler_,
      .imageView = back_depth_view_,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    };

    const VkDescriptorImageInfo mask_image_info = {
      .sampler = mask_sampler_,
      .imageView = mask_view_,
      .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
    };

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
      VkWriteDescriptorSet write_descriptor[] = {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = textures_descriptor_sets_[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &ray_enter_image_info,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = textures_descriptor_sets_[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &ray_leave_image_info,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = textures_descriptor_sets_[i],
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &back_depth_image_info,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr,
        },
      };

      vkUpdateDescriptorSets(engine.get_device(), 3, write_descriptor, 0, nullptr);

      VkWriteDescriptorSet mask_write_descriptor = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = frontface_descriptor_sets_[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &mask_image_info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
      };

      vkUpdateDescriptorSets(engine.get_device(), 1, &mask_write_descriptor, 0, nullptr);
    }
  }

  void CSGPipeline::render_depth(VkCommandBuffer& command_buffer, scene::Mesh& mesh,
                                 scene::Mesh& substractive_mesh)
  {
    auto& engine = core::Engine::get_singleton();
    auto extent = engine.get_swapchain_extent();

    const VkRect2D render_area = {
      .offset = { 0, 0 },
      .extent = extent,
    };

    const VkClearValue depth_clear_value = { .depthStencil = { .depth = 1.0f, .stencil = 0 } };

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, depth_pipeline_);
    vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
                       mesh.cframe.to_matrix().data());

    int one = 1;
    vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 64, 4, &one);

    vkCmdSetDepthTestEnable(command_buffer, VK_TRUE);
    vkCmdSetDepthWriteEnable(command_buffer, VK_TRUE);
    vkCmdSetDepthCompareOp(command_buffer, VK_COMPARE_OP_LESS);
    vkCmdSetStencilTestEnable(command_buffer, VK_FALSE);

    VkDeviceSize offset = 0;
    VkBuffer vertex_buffer = mesh.get_vertex_buffer();
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
    vkCmdBindIndexBuffer(command_buffer, mesh.get_index_buffer(), offset, VK_INDEX_TYPE_UINT32);

    const VkRenderingAttachmentInfo ray_enter_attachment = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .pNext = nullptr,
      .imageView = ray_enter_view_,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .resolveImageView = VK_NULL_HANDLE,
      .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = depth_clear_value,
    };

    const VkRenderingInfo ray_enter_rendering_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .pNext = nullptr,
      .flags = 0,
      .renderArea = render_area,
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = 0,
      .pColorAttachments = nullptr,
      .pDepthAttachment = &ray_enter_attachment,
      .pStencilAttachment = nullptr,
    };

    vkCmdBeginRendering(command_buffer, &ray_enter_rendering_info);

    vkCmdSetCullMode(command_buffer, VK_CULL_MODE_BACK_BIT);
    vkCmdDrawIndexed(command_buffer, mesh.get_index_count(), 1, 0, 0, 0);

    vkCmdEndRendering(command_buffer);

    const VkRenderingAttachmentInfo ray_leave_attachment = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .pNext = nullptr,
      .imageView = ray_leave_view_,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .resolveImageView = VK_NULL_HANDLE,
      .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = depth_clear_value,
    };

    const VkRenderingInfo ray_leave_rendering_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .pNext = nullptr,
      .flags = 0,
      .renderArea = render_area,
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = 0,
      .pColorAttachments = nullptr,
      .pDepthAttachment = &ray_leave_attachment,
      .pStencilAttachment = nullptr,
    };

    vkCmdBeginRendering(command_buffer, &ray_leave_rendering_info);

    vkCmdSetCullMode(command_buffer, VK_CULL_MODE_FRONT_BIT);
    vkCmdDrawIndexed(command_buffer, mesh.get_index_count(), 1, 0, 0, 0);

    vkCmdEndRendering(command_buffer);

    const VkRenderingAttachmentInfo back_depth_attachment = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .pNext = nullptr,
      .imageView = back_depth_view_,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .resolveImageView = VK_NULL_HANDLE,
      .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = depth_clear_value,
    };

    const VkRenderingInfo back_depth_rendering_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .pNext = nullptr,
      .flags = 0,
      .renderArea = render_area,
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = 0,
      .pColorAttachments = nullptr,
      .pDepthAttachment = &back_depth_attachment,
      .pStencilAttachment = nullptr,
    };

    vkCmdBeginRendering(command_buffer, &back_depth_rendering_info);

    vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 64,
                       substractive_mesh.cframe.to_matrix().data());

    vkCmdSetCullMode(command_buffer, VK_CULL_MODE_BACK_BIT);

    vertex_buffer = substractive_mesh.get_vertex_buffer();
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
    vkCmdBindIndexBuffer(command_buffer, substractive_mesh.get_index_buffer(), offset,
                         VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(command_buffer, substractive_mesh.get_index_count(), 1, 0, 0, 0);

    vkCmdEndRendering(command_buffer);
  }
} // namespace gfx
