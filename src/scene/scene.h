#pragma once

#include <vulkan/vulkan.h>

#include "scene/camera.h"
#include "scene/instance.h"
#include "scene/visitor.h"

namespace scene
{
  class Scene : public Instance
  {
  public:
    Scene() = default;

    void set_parent(Instance*) override;

    void accept(Visitor& visitor) override;

    VkImage get_skybox_image() const;
    VkImageView get_skybox_image_view() const;
    VkSampler get_skybox_sampler() const;

    Camera* current_camera = nullptr;

  private:
    VkImage skybox_image_ = VK_NULL_HANDLE;
    VkImageView skybox_image_view_ = VK_NULL_HANDLE;
    VkSampler skybox_sampler_ = VK_NULL_HANDLE;
  };
} // namespace scene

#include "scene/scene.hxx"
