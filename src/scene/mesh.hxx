#include "scene/mesh.h"

namespace scene
{
  inline void Mesh::accept(Visitor& visitor) { visitor(*this); }
  inline VkBuffer Mesh::get_vertex_buffer() const { return vertex_buffer_; }
  inline VkBuffer Mesh::get_index_buffer() const { return index_buffer_; }
  inline uint32_t Mesh::get_vertex_count() const { return vertex_count_; }
  inline uint32_t Mesh::get_index_count() const { return index_count_; }
} // namespace scene
