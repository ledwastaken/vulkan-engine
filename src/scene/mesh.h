#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "scene/object.h"
#include "scene/visitor.h"
#include "types/vector2.h"
#include "types/vector3.h"

namespace scene
{
  struct Vertex
  {
    types::Vector3 position;
    types::Vector3 normal;
    types::Vector2 uv;
  };

  class Mesh : public Object
  {
  public:
    Mesh() = default;

    virtual ~Mesh();

    virtual void load_mesh_data(const std::vector<Vertex>& vertices,
                                const std::vector<uint32_t>& indices);
    virtual void load_mesh_from_file(const std::string& path);
    virtual void reset();

    void accept(Visitor& visitor) override;

    VkBuffer get_vertex_buffer() const;
    VkBuffer get_index_buffer() const;
    uint32_t get_vertex_count() const;
    uint32_t get_index_count() const;

  private:
    void create_vertex_buffer(const std::vector<Vertex>& vertices);
    void create_index_vertex(const std::vector<uint32_t>& indices);

    VkBuffer vertex_buffer_ = VK_NULL_HANDLE;
    VkBuffer index_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertex_buffer_memory_ = VK_NULL_HANDLE;
    VkDeviceMemory index_buffer_memory_ = VK_NULL_HANDLE;
    uint32_t vertex_count_ = 0;
    uint32_t index_count_ = 0;
  };
} // namespace scene

#include "scene/mesh.hxx"
