#include "render/deferred-renderer.h"

#include <SDL3/SDL.h>

#include "core/engine.h"
#include "core/scene-manager.h"
#include "gfx/skybox-pipeline.h"
#include "types/matrix4.h"

namespace render
{
  void DeferredRenderer::init()
  {
    // FIXME
  }

  void DeferredRenderer::draw(VkImageView image_view, VkCommandBuffer command_buffer)
  {
    auto& engine = core::Engine::get_singleton();
    auto& scene_manager = core::SceneManager::get_singleton();
    auto scene = scene_manager.get_current_scene();

    if (!scene || !scene->current_camera)
      return;

    int width, height;
    if (!SDL_GetWindowSize(engine.get_window(), &width, &height))
      throw std::runtime_error(SDL_GetError());

    float ratio = static_cast<float>(width) / static_cast<float>(height);
    auto& camera = *scene->current_camera;
    auto view = camera.cframe.invert().to_matrix();
    auto projection = types::Matrix4::perpective(camera.field_of_view, ratio, 0.1f, 100.0f);

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

    if (scene->get_skybox_image())
    {
      auto& skybox_renderer = gfx::SkyboxPipeline::get_singleton();

      const gfx::SkyboxData skybox_data = {
        .image = scene->get_skybox_image(),
        .image_view = scene->get_skybox_image_view(),
        .sampler = scene->get_skybox_sampler(),
      };

      skybox_renderer.draw(image_view, command_buffer, view, projection, skybox_data);
    }

    // Visitor::operator()(*scene);
    // TODO: Visit the scene tree instead

    vkEndCommandBuffer(command_buffer);
  }

  void DeferredRenderer::free()
  {
    // FIXME
  }
} // namespace render
