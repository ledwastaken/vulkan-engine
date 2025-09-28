#include "scene/scene.h"

namespace scene
{
  inline void Scene::accept(Visitor& visitor) { visitor(*this); }
} // namespace scene
