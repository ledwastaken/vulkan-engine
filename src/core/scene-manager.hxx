#include "core/scene-manager.h"

namespace core
{
  inline void SceneManager::set_current_scene(scene::Scene* scene) { current_scene_ = scene; }
  inline scene::Scene* SceneManager::get_current_scene() const { return current_scene_; }
} // namespace core
