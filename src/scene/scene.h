#pragma once

#include <string>

#include <vulkan/vulkan.h>

#include "scene/camera.h"
#include "scene/instance.h"
#include "scene/mesh.h"
#include "scene/visitor.h"

namespace scene
{
  class Scene : public Instance
  {
  public:
    Scene() = default;
    ~Scene();

    void set_parent(Instance*) override;

    void accept(Visitor& visitor) override;

    void load_skybox(const std::string& right, const std::string& left, const std::string& top,
                     const std::string& bottom, const std::string& front, const std::string& back);

    VkImage get_skybox_image() const;
    VkImageView get_skybox_image_view() const;
    VkSampler get_skybox_sampler() const;

    Camera* current_camera = nullptr;
    Mesh* mesh = nullptr;
    Mesh* substractive_mesh = nullptr;

  private:
    VkImage skybox_image_ = VK_NULL_HANDLE;
    VkImageView skybox_image_view_ = VK_NULL_HANDLE;
    VkSampler skybox_sampler_ = VK_NULL_HANDLE;
    VkDeviceMemory skybox_image_memory_ = VK_NULL_HANDLE;
  };
} // namespace scene

#include "scene/scene.hxx"
