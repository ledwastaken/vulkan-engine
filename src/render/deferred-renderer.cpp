#include "render/deferred-renderer.h"

#include <cmath>

#include <SDL3/SDL.h>
#include <imgui.h>

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

    auto dt = ImGui::GetIO().DeltaTime;
    auto speed = ImGui::IsKeyDown(ImGuiKey_LeftShift) ? 1 : 5;

    if (ImGui::IsKeyDown(ImGuiKey_Z))
      camera.cframe += camera.cframe.get_look_vector() * dt * speed;
    if (ImGui::IsKeyDown(ImGuiKey_S))
      camera.cframe += -camera.cframe.get_look_vector() * dt * speed;
    if (ImGui::IsKeyDown(ImGuiKey_Q))
      camera.cframe += -camera.cframe.get_right_vector() * dt * speed;
    if (ImGui::IsKeyDown(ImGuiKey_D))
      camera.cframe += camera.cframe.get_right_vector() * dt * speed;
    if (ImGui::IsKeyDown(ImGuiKey_A))
      camera.cframe += -camera.cframe.get_up_vector() * dt * speed;
    if (ImGui::IsKeyDown(ImGuiKey_E))
      camera.cframe += camera.cframe.get_up_vector() * dt * speed;
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
      ImVec2 delta = ImGui::GetIO().MouseDelta;

      auto position = camera.cframe.get_position();

      camera.cframe += -position;
      camera.cframe = types::CFrame::from_axis_angle(types::Vector3(0, 1, 0), -0.006f * delta.x)
          * camera.cframe;
      camera.cframe += position;

      camera.cframe *= types::CFrame::from_axis_angle(types::Vector3(1, 0, 0), -0.006f * delta.y);
    }

    auto view = camera.cframe.invert().to_matrix();
    auto projection = types::Matrix4::perspective(camera.field_of_view, ratio, 0.1f, 100.0f);

    // if (scene->get_skybox_image())
    {
      auto& skybox_renderer = gfx::SkyboxPipeline::get_singleton();

      const gfx::SkyboxData skybox_data = {
        .image = scene->get_skybox_image(),
        .image_view = scene->get_skybox_image_view(),
        .sampler = scene->get_skybox_sampler(),
      };

      skybox_renderer.draw(image_view, command_buffer, view, projection, skybox_data);
    }

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

    pbr_pipeline.draw(image_view_, command_buffer_, view_, projection_, mesh);
  }
} // namespace render
