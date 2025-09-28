#include "scene/mesh.h"

#include <cstring>

#include "core/engine.h"

namespace scene
{
  Mesh::~Mesh()
  {
    auto& engine = core::Engine::get_singleton();

    if (vertex_buffer_ != VK_NULL_HANDLE)
      vkDestroyBuffer(engine.get_device(), vertex_buffer_, nullptr);

    if (vertex_buffer_memory_ != VK_NULL_HANDLE)
      vkFreeMemory(engine.get_device(), vertex_buffer_memory_, nullptr);
  }

  void Mesh::load_vertices(const std::vector<Vertex>& vertices)
  {
    auto& engine = core::Engine::get_singleton();
    auto buffer_size = vertices.size() * sizeof(Vertex);

    const VkBufferCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = buffer_size,
      .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
    };

    if (vertex_buffer_ != VK_NULL_HANDLE)
      vkDestroyBuffer(engine.get_device(), vertex_buffer_, nullptr);

    if (vertex_buffer_memory_ != VK_NULL_HANDLE)
      vkFreeMemory(engine.get_device(), vertex_buffer_memory_, nullptr);

    VkResult result = vkCreateBuffer(engine.get_device(), &create_info, nullptr, &vertex_buffer_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create buffer");

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(engine.get_device(), vertex_buffer_, &memory_requirements);

    auto flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const VkMemoryAllocateInfo allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = nullptr,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = engine.find_memory_type(memory_requirements.memoryTypeBits, flags),
    };

    result = vkAllocateMemory(engine.get_device(), &allocate_info, nullptr, &vertex_buffer_memory_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to allocate device memory");

    result = vkBindBufferMemory(engine.get_device(), vertex_buffer_, vertex_buffer_memory_, 0);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to bind buffer memory");

    void* data;
    result = vkMapMemory(engine.get_device(), vertex_buffer_memory_, 0, buffer_size, 0, &data);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to map buffer memory");

    std::memcpy(data, vertices.data(), buffer_size);
    vkUnmapMemory(engine.get_device(), vertex_buffer_memory_);
  }

  void Mesh::reset()
  {
    auto& engine = core::Engine::get_singleton();

    if (vertex_buffer_ != VK_NULL_HANDLE)
      vkDestroyBuffer(engine.get_device(), vertex_buffer_, nullptr);

    if (vertex_buffer_memory_ != VK_NULL_HANDLE)
      vkFreeMemory(engine.get_device(), vertex_buffer_memory_, nullptr);

    vertex_buffer_ = VK_NULL_HANDLE;
    vertex_buffer_memory_ = VK_NULL_HANDLE;
  }
} // namespace scene
