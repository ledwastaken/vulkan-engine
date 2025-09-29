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

    virtual void load_vertices(const std::vector<Vertex>& vertices);
    virtual void reset();

    void accept(Visitor& visitor) override;

    VkBuffer get_vertex_buffer() const;
    uint32_t get_vertex_count() const;

  private:
    VkBuffer vertex_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertex_buffer_memory_ = VK_NULL_HANDLE;
    uint32_t vertex_count_ = 0;
  };
} // namespace scene

#include "scene/mesh.hxx"
