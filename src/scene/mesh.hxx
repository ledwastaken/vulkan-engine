#include "scene/mesh.h"

namespace scene
{
  inline void Mesh::accept(Visitor& visitor) { visitor(*this); }
} // namespace scene
