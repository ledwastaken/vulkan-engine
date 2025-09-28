#pragma once

#include "scene/object.h"

namespace scene
{
  class Camera : public Object
  {
  public:
    Camera() = default;

    void accept(Visitor& visitor) override;
  };
} // namespace scene

#include "scene/camera.hxx"
