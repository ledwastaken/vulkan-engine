#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "scene/object.h"
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

  private:
    VkBuffer vertex_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertex_buffer_memory_ = VK_NULL_HANDLE;
  };
} // namespace scene
