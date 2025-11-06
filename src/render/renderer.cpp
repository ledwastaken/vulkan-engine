#include "render/renderer.h"

#include <cmath>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "core/engine.h"
#include "core/scene-manager.h"
#include "gfx/back-face-pipeline.h"
#include "gfx/csg-pipeline.h"
#include "gfx/skybox-pipeline.h"
#include "scene/mesh.h"

using namespace core;

namespace render
{
  void Renderer::init()
  {
    auto& engine = Engine::get_singleton();

    const VkExtent3D image_extent = {
      .width = 800,
      .height = 600,
      .depth = 1,
    };

    const VkImageCreateInfo back_face_image_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R32G32B32A32_SFLOAT,
      .extent = image_extent,
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    engine.create_image(back_face_image_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        back_face_image_, back_face_image_memory_);
    engine.create_image(depth_image_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_image_,
                        depth_image_memory_);

    const VkComponentMapping color_components = {
      .r = VK_COMPONENT_SWIZZLE_R,
      .g = VK_COMPONENT_SWIZZLE_G,
      .b = VK_COMPONENT_SWIZZLE_B,
      .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    const VkComponentMapping depth_components = {
      .r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    const VkImageSubresourceRange color_subresource_range = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    };

    const VkImageSubresourceRange depth_subresource_range = {
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    };

    const VkImageViewCreateInfo back_face_view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = back_face_image_,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R32G32B32A32_SFLOAT,
      .components = color_components,
      .subresourceRange = color_subresource_range,
    };

    const VkImageViewCreateInfo depth_view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = depth_image_,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_D32_SFLOAT,
      .components = depth_components,
      .subresourceRange = depth_subresource_range,
    };

    VkResult result = vkCreateImageView(engine.get_device(), &back_face_view_info, nullptr,
                                        &back_face_image_view_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create image view");

    result = vkCreateImageView(engine.get_device(), &depth_view_info, nullptr, &depth_image_view_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create image view");

    const VkSamplerCreateInfo back_face_sampler_create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 0.0f,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_NEVER,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
    };

    const VkSamplerCreateInfo depth_sampler_create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 0.0f,
      .compareEnable = VK_TRUE,
      .compareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
      .unnormalizedCoordinates = VK_FALSE,
    };

    result = vkCreateSampler(engine.get_device(), &back_face_sampler_create_info, nullptr,
                             &back_face_sampler_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create sampler");

    const TransitionLayout color_transition_layout = {
      .src_access = 0,
      .dst_access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      .dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
      .old_layout = VK_IMAGE_LAYOUT_UNDEFINED,
      .new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    const TransitionLayout depth_transition_layout = {
      .src_access = 0,
      .dst_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      .src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      .dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .old_layout = VK_IMAGE_LAYOUT_UNDEFINED,
      .new_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
    };

    engine.transition_image_layout(back_face_image_, VK_FORMAT_R32G32B32A32_SFLOAT, 1,
                                   color_transition_layout);
    engine.transition_image_layout(depth_image_, VK_FORMAT_D32_SFLOAT, 1, depth_transition_layout);
  }

  void Renderer::draw(VkImageView image_view, VkCommandBuffer command_buffer)
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

  void Renderer::free()
  {
    auto& engine = Engine::get_singleton();

    vkDestroySampler(engine.get_device(), back_face_sampler_, nullptr);
    vkDestroyImageView(engine.get_device(), back_face_image_view_, nullptr);
    vkDestroyImage(engine.get_device(), back_face_image_, nullptr);
    vkFreeMemory(engine.get_device(), back_face_image_memory_, nullptr);

    vkDestroySampler(engine.get_device(), depth_sampler_, nullptr);
    vkDestroyImageView(engine.get_device(), depth_image_view_, nullptr);
    vkDestroyImage(engine.get_device(), depth_image_, nullptr);
    vkFreeMemory(engine.get_device(), depth_image_memory_, nullptr);
  }

  void Renderer::operator()(scene::Mesh& mesh)
  {
    auto& csg_pipeline = gfx::CSGPipeline::get_singleton();

    csg_pipeline.draw(image_view_, command_buffer_, view_, projection_, mesh);
  }
} // namespace render
