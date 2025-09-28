#include "scene/camera.h"

namespace scene
{
  inline void Camera::accept(Visitor& visitor) { visitor(*this); }
} // namespace scene
