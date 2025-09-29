#include "scene/mesh.h"

namespace scene
{
  inline void Mesh::accept(Visitor& visitor) { visitor(*this); }
  inline VkBuffer Mesh::get_vertex_buffer() const { return vertex_buffer_; }
  inline uint32_t Mesh::get_vertex_count() const { return vertex_count_; }

} // namespace scene
