#pragma once

#include "scene/camera.h"
#include "scene/instance.h"

namespace scene
{
  class Scene : public Instance
  {
  public:
    Scene() = default;

    void set_parent(Instance*) override;

    Camera* current_camera = nullptr;
  };
} // namespace scene
