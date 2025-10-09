#include "scene/scene.h"

namespace scene
{
  inline void Scene::accept(Visitor& visitor) { visitor(*this); }
  inline VkImage Scene::get_skybox_image() const { return skybox_image_; }
  inline VkImageView Scene::get_skybox_image_view() const { return skybox_image_view_; }
  inline VkSampler Scene::get_skybox_sampler() const { return skybox_sampler_; }
} // namespace scene
