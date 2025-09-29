#include "render/renderer.h"

#include "core/engine.h"
#include "scene/mesh.h"

using namespace scene;

namespace render
{
  void Renderer::operator()(scene::Scene& scene, uint32_t image_index)
  {
    auto& engine = core::Engine::get_singleton();
    command_buffer_ = engine.get_command_buffer(image_index);

    const VkCommandBufferBeginInfo command_buffer_begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = 0,
      .pInheritanceInfo = nullptr,
    };

    VkResult result = vkResetCommandBuffer(command_buffer_, 0);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to reset command buffer");

    result = vkBeginCommandBuffer(command_buffer_, &command_buffer_begin_info);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to begin command buffer recording");

    const VkRect2D render_area = {
      .offset = { 0, 0 },
      .extent = engine.get_swapchain_extent(),
    };

    const VkClearValue clear_value = { { { 0.3f, 0.3f, 0.3f, 1.0f } } };

    const VkRenderPassBeginInfo renderpass_begin_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = nullptr,
      .renderPass = engine.get_renderpass(),
      .framebuffer = engine.get_frambuffer(image_index),
      .renderArea = render_area,
      .clearValueCount = 1,
      .pClearValues = &clear_value,
    };

    vkCmdBeginRenderPass(command_buffer_, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      engine.get_graphics_pipeline());

    VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(engine.get_swapchain_extent().width),
      .height = static_cast<float>(engine.get_swapchain_extent().height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
      .offset = { 0, 0 },
      .extent = engine.get_swapchain_extent(),
    };

    vkCmdSetViewport(command_buffer_, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer_, 0, 1, &scissor);

    this->operator()(scene);

    vkCmdEndRenderPass(command_buffer_);
    result = vkEndCommandBuffer(command_buffer_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to end command buffer recording");
  }

  void Renderer::operator()(Mesh& mesh)
  {
    auto vertex_buffer = mesh.get_vertex_buffer();

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command_buffer_, 0, 1, &vertex_buffer, &offset);
    vkCmdDraw(command_buffer_, mesh.get_vertex_count(), 1, 0, 0);

    Visitor::operator()(mesh);
  }
} // namespace render
