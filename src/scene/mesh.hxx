#include "scene/mesh.h"

namespace scene
{
  inline void Mesh::accept(Visitor& visitor) { visitor(*this); }
  inline VkBuffer Mesh::get_vertex_buffer() const { return vertex_buffer_; }
} // namespace scene
