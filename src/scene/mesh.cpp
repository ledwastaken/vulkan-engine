#include "scene/mesh.h"

#include <cstring>

#include "core/engine.h"

using namespace core;

namespace scene
{
  Mesh::Mesh() { create_position_image(); }

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

    vkDestroySampler(engine.get_device(), position_sampler_, nullptr);
    vkDestroyImageView(engine.get_device(), position_image_view_, nullptr);
    vkDestroyImage(engine.get_device(), position_image_, nullptr);
    vkFreeMemory(engine.get_device(), position_image_memory_, nullptr);
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

  void Mesh::create_position_image()
  {
    auto& engine = Engine::get_singleton();

    const VkExtent3D image_extent = {
      .width = 1024,
      .height = 1024,
      .depth = 1,
    };

    const VkImageCreateInfo image_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R32G32B32_SFLOAT,
      .extent = image_extent,
      .mipLevels = 1,
      .arrayLayers = 6,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    engine.create_image(image_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, position_image_,
                        position_image_memory_);

    const VkComponentMapping components = {
      .r = VK_COMPONENT_SWIZZLE_R,
      .g = VK_COMPONENT_SWIZZLE_G,
      .b = VK_COMPONENT_SWIZZLE_B,
      .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    const VkImageSubresourceRange subresource_range = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 6,
    };

    const VkImageViewCreateInfo view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = position_image_,
      .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
      .format = VK_FORMAT_R32G32B32_SFLOAT,
      .components = components,
      .subresourceRange = subresource_range,
    };

    VkResult result =
        vkCreateImageView(engine.get_device(), &view_info, nullptr, &position_image_view_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create depth image view");

    const VkSamplerCreateInfo sampler_create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 0.0f,
      .compareEnable = VK_TRUE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
    };

    result =
        vkCreateSampler(engine.get_device(), &sampler_create_info, nullptr, &position_sampler_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create sampler");

    const TransitionLayout transition_layout = {
      .src_access = 0,
      .dst_access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
      .src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      .dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
      .old_layout = VK_IMAGE_LAYOUT_UNDEFINED,
      .new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    engine.transition_image_layout(position_image_, VK_FORMAT_R32G32B32_SFLOAT, 6,
                                   transition_layout);
  }
} // namespace scene
