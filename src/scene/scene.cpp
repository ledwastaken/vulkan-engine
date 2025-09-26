#include "scene/scene.h"

#include <stdexcept>

namespace scene
{
  void Scene::set_parent(Instance* parent)
  {
    throw std::runtime_error("cannot set the parent of a scene");
  }
} // namespace scene
