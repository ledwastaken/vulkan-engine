#pragma once

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

    Camera* current_camera = nullptr;
  };
} // namespace scene

#include "scene/scene.hxx"
