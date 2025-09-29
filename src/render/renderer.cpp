#include "render/renderer.h"

#include "core/engine.h"
#include "scene/mesh.h"

using namespace scene;

namespace render
{
  void Renderer::operator()(scene::Scene& scene, uint32_t image_index)
  {
    auto& engine = core::Engine::get_singleton();
    auto command_buffer = engine.get_command_buffer(image_index);

    const VkCommandBufferBeginInfo command_buffer_begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = 0,
      .pInheritanceInfo = nullptr,
    };

    VkResult result = vkResetCommandBuffer(command_buffer, 0);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to reset command buffer");

    result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to begin command buffer recording");

    const VkRect2D render_area = {
      .offset = { 0, 0 },
      .extent = engine.get_swapchain_extent(),
    };

    const VkClearValue clear_value = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };

    const VkRenderPassBeginInfo renderpass_begin_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = nullptr,
      .renderPass = engine.get_renderpass(),
      .framebuffer = engine.get_frambuffer(image_index),
      .renderArea = render_area,
      .clearValueCount = 1,
      .pClearValues = &clear_value,
    };

    vkCmdBeginRenderPass(command_buffer, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // TODO: render objects

    vkCmdEndRenderPass(command_buffer);
    result = vkEndCommandBuffer(command_buffer);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to end command buffer recording");
  }

  void Renderer::operator()(Mesh& mesh)
  {
    // FIXME: render mesh
    Visitor::operator()(mesh);
  }
} // namespace render
