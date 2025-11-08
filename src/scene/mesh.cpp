#include "scene/mesh.h"

#include <cstring>
#include <fstream>
#include <sstream>

#include "core/engine.h"

using namespace core;

namespace scene
{
  Mesh::~Mesh()
  {
    auto& engine = Engine::get_singleton();

    if (vertex_buffer_ != VK_NULL_HANDLE)
      vkDestroyBuffer(engine.get_device(), vertex_buffer_, nullptr);

    if (index_buffer_ != VK_NULL_HANDLE)
      vkDestroyBuffer(engine.get_device(), index_buffer_, nullptr);

    if (vertex_buffer_memory_ != VK_NULL_HANDLE)
      vkFreeMemory(engine.get_device(), vertex_buffer_memory_, nullptr);

    if (index_buffer_memory_ != VK_NULL_HANDLE)
      vkFreeMemory(engine.get_device(), index_buffer_memory_, nullptr);

    vkDeviceWaitIdle(engine.get_device());
  }

  void Mesh::load_mesh_data(const std::vector<Vertex>& vertices,
                            const std::vector<uint32_t>& indices)
  {
    create_vertex_buffer(vertices);
    create_index_vertex(indices);
  }

  void Mesh::reset()
  {
    auto& engine = Engine::get_singleton();

    if (vertex_buffer_ != VK_NULL_HANDLE)
      vkDestroyBuffer(engine.get_device(), vertex_buffer_, nullptr);

    if (index_buffer_ != VK_NULL_HANDLE)
      vkDestroyBuffer(engine.get_device(), index_buffer_, nullptr);

    if (vertex_buffer_memory_ != VK_NULL_HANDLE)
      vkFreeMemory(engine.get_device(), vertex_buffer_memory_, nullptr);

    if (index_buffer_memory_ != VK_NULL_HANDLE)
      vkFreeMemory(engine.get_device(), index_buffer_memory_, nullptr);

    vertex_buffer_ = VK_NULL_HANDLE;
    index_buffer_ = VK_NULL_HANDLE;
    vertex_buffer_memory_ = VK_NULL_HANDLE;
    index_buffer_memory_ = VK_NULL_HANDLE;
  }

  void Mesh::create_vertex_buffer(const std::vector<Vertex>& vertices)
  {
    auto& engine = Engine::get_singleton();
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

    vertex_count_ = vertices.size();
  }

  void Mesh::load_mesh_from_file(const std::string& path)
  {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indicies;

    std::ifstream file(path);
    if (file.is_open())
    {
      int start = 0;

      std::vector<types::Vector3> positions;
      std::vector<types::Vector3> normals;

      std::string line;
      while (std::getline(file, line))
      {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v")
        {
          types::Vector3 pos;
          iss >> pos.x >> pos.y >> pos.z;
          positions.push_back(pos);
        }
        else if (prefix == "vn")
        {
          types::Vector3 normal;
          iss >> normal.x >> normal.y >> normal.z;
          normals.push_back(normal);
        }
        else if (prefix == "f")
        {
          std::string v1, v2, v3, v4;
          iss >> v1 >> v2 >> v3 >> v4;

          auto faceData = v4.empty() ? std::vector<std::string>{ v1, v2, v3 }
                                     : std::vector<std::string>{ v1, v2, v3, v4 };

          for (const std::string& vert : faceData)
          {
            std::istringstream vstream(vert);
            std::string p, t, n;
            std::getline(vstream, p, '/');
            std::getline(vstream, t, '/');
            std::getline(vstream, n, '/');

            Vertex vertex = { positions[std::stoi(p) - 1], normals[std::stoi(n) - 1] };

            vertices.push_back(vertex);
          }

          if (v4.empty())
          {
            indicies.push_back(start);
            indicies.push_back(start + 1);
            indicies.push_back(start + 2);

            start += 3;
          }
          else
          {
            indicies.push_back(start);
            indicies.push_back(start + 1);
            indicies.push_back(start + 2);
            indicies.push_back(start + 2);
            indicies.push_back(start + 3);
            indicies.push_back(start);

            start += 4;
          }
        }
      }
    }

    load_mesh_data(vertices, indicies);
  }

  void Mesh::create_index_vertex(const std::vector<uint32_t>& indices)
  {
    auto& engine = Engine::get_singleton();
    auto buffer_size = indices.size() * sizeof(uint32_t);

    const VkBufferCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = buffer_size,
      .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
    };

    if (index_buffer_ != VK_NULL_HANDLE)
      vkDestroyBuffer(engine.get_device(), index_buffer_, nullptr);

    if (index_buffer_memory_ != VK_NULL_HANDLE)
      vkFreeMemory(engine.get_device(), index_buffer_memory_, nullptr);

    VkResult result = vkCreateBuffer(engine.get_device(), &create_info, nullptr, &index_buffer_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create buffer");

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(engine.get_device(), index_buffer_, &memory_requirements);

    auto flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const VkMemoryAllocateInfo allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = nullptr,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = engine.find_memory_type(memory_requirements.memoryTypeBits, flags),
    };

    result = vkAllocateMemory(engine.get_device(), &allocate_info, nullptr, &index_buffer_memory_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to allocate device memory");

    result = vkBindBufferMemory(engine.get_device(), index_buffer_, index_buffer_memory_, 0);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to bind buffer memory");

    void* data;
    result = vkMapMemory(engine.get_device(), index_buffer_memory_, 0, buffer_size, 0, &data);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to map buffer memory");

    std::memcpy(data, indices.data(), buffer_size);
    vkUnmapMemory(engine.get_device(), index_buffer_memory_);

    index_count_ = indices.size();
  }
} // namespace scene
