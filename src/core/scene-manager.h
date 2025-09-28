#pragma once

#include <vector>

#include "misc/singleton.h"
#include "scene/scene.h"

namespace core
{
  using SceneInitializer = void (*)(scene::Scene&);

  class SceneManager : public misc::Singleton<SceneManager>
  {
    // Give Singleton<SceneManager> access to SceneManager's private constructor
    friend class Singleton<SceneManager>;

  private:
    SceneManager() = default;

  public:
    void set_current_scene(scene::Scene* scene);
    scene::Scene* get_current_scene() const;

  private:
    scene::Scene* current_scene_;
  };
} // namespace core

#include "core/scene-manager.hxx"
