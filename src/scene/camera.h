#pragma once

#include "scene/object.h"
#include "types/matrix4.h"

namespace scene
{
  class Camera : public Object
  {
  public:
    Camera() = default;

    types::Matrix4 compute_view_matrix() const;
    types::Matrix4 compute_projection_matrix() const;

    void accept(Visitor& visitor) override;

    float field_of_view = 70.0f;
  };
} // namespace scene

#include "scene/camera.hxx"
