#include "core/asset-manager.h"

#include <cstring>
#include <stdexcept>

#include <stb/stb_image.h>

#include "core/engine.h"

namespace core
{
  ImageData* AssetManager::load_image(const std::string& path, const std::string& name)
  {
    auto it = images_.find(name);
    if (it != images_.end())
      return it->second;

    int width, height, channels;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
      throw std::runtime_error("failed to load image");

    VkDeviceSize image_size = width * height * 4;
    auto image_data = new ImageData();

    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;

    auto& engine = Engine::get_singleton();
    auto device = engine.get_device();

    // TODO: Create a create_buffer method someday
    const VkBufferCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = image_size,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
    };

    VkResult result = vkCreateBuffer(device, &create_info, nullptr, &staging_buffer);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create buffer");

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, staging_buffer, &memory_requirements);

    auto flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const VkMemoryAllocateInfo buffer_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = nullptr,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = engine.find_memory_type(memory_requirements.memoryTypeBits, flags),
    };

    result = vkAllocateMemory(device, &buffer_allocate_info, nullptr, &staging_memory);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to allocate device memory");

    result = vkBindBufferMemory(device, staging_buffer, staging_memory, 0);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to bind buffer memory");

    void* data;
    result = vkMapMemory(device, staging_memory, 0, image_size, 0, &data);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to map buffer memory");

    std::memcpy(data, pixels, image_size);
    vkUnmapMemory(device, staging_memory);

    images_.insert({ name, image_data });
    stbi_image_free(pixels);

    const VkExtent3D image_extent = {
      .width = width,
      .height = height,
      .depth = 4,
    };

    const VkImageCreateInfo image_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_SRGB,
      .extent = image_extent,
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    result = vkCreateImage(device, &image_create_info, nullptr, &image_data->image);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create image");

    vkGetImageMemoryRequirements(device, image_data->image, &memory_requirements);

    auto flag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    const VkMemoryAllocateInfo image_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = nullptr,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = engine.find_memory_type(memory_requirements.memoryTypeBits, flags),
    };

    result = vkAllocateMemory(device, &image_allocate_info, nullptr, &image_data->memory);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to allocate device memory");

    result = vkBindImageMemory(device, image_data->image, image_data->memory, 0);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to bind buffer memory");

    // FIXME: Transfer data

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_memory, nullptr);

    const VkComponentMapping components = {
      .r = VK_COMPONENT_SWIZZLE_R,
      .g = VK_COMPONENT_SWIZZLE_G,
      .b = VK_COMPONENT_SWIZZLE_B,
      .a = VK_COMPONENT_SWIZZLE_A,
    };

    const VkImageSubresourceRange subresource_range = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    };

    const VkImageViewCreateInfo image_view_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = image_data->image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_SRGB,
      .components = components,
      .subresourceRange = subresource_range,
    };

    result = vkCreateImageView(device, &image_view_create_info, nullptr, &image_data->image_view);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create image view");

    const VkSamplerCreateInfo sampler_create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_TRUE,
      .maxAnisotropy = 16.0f,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
    };

    result = vkCreateSampler(device, &sampler_create_info, nullptr, &image_data->sampler);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create sampler");
  }

  ImageData* AssetManager::find_image(const std::string& name)
  {
    auto it = images_.find(name);
    return it != images_.end() ? it->second : nullptr;
  }

  void AssetManager::destroy_image(const std::string& name)
  {
    auto& engine = Engine::get_singleton();
    auto device = engine.get_device();
    auto it = images_.find(name);

    if (it != images_.end())
    {
      auto image_data = it->second;

      vkDestroySampler(device, image_data->sampler, nullptr);
      vkDestroyImageView(device, image_data->image_view, nullptr);
      vkDestroyImage(device, image_data->image, nullptr);
      vkFreeMemory(device, image_data->memory, nullptr);

      delete image_data;
      images_.erase(name);
    }
  }

  void AssetManager::free()
  {
    for (auto it = images_.begin(); it != images_.end(); it++)
      destroy_image(it->first);
  }
} // namespace core
