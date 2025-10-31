#include "render/deferred-renderer.h"

#include <SDL3/SDL.h>

#include "core/engine.h"
#include "core/scene-manager.h"
#include "gfx/pbr-pipeline.h"
#include "gfx/skybox-pipeline.h"
#include "scene/mesh.h"

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

    // if (scene->get_skybox_image())
    // {
    //   auto& skybox_renderer = gfx::SkyboxPipeline::get_singleton();

    //   const gfx::SkyboxData skybox_data = {
    //     .image = scene->get_skybox_image(),
    //     .image_view = scene->get_skybox_image_view(),
    //     .sampler = scene->get_skybox_sampler(),
    //   };

    //   skybox_renderer.draw(image_view, command_buffer, view, projection, skybox_data);
    // }

    image_view_ = image_view;
    command_buffer_ = command_buffer;
    view_ = view;
    projection_ = projection;

    Visitor::operator()(*scene);
  }

  void DeferredRenderer::free()
  {
    // FIXME
  }

  void DeferredRenderer::operator()(scene::Mesh& mesh)
  {
    auto& pbr_pipeline = gfx::PhysicallyBasedRenderPipeline::get_singleton();

    pbr_pipeline.draw(image_view_, command_buffer_, view_, projection_, mesh.get_index_buffer());
  }
} // namespace render
