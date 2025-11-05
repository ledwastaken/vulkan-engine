#include "scene/scene.h"

#include <array>
#include <cstring>
#include <stdexcept>

#include <stb/stb_image.h>

#include "core/engine.h"

namespace scene
{
  Scene::~Scene()
  {
    if (skybox_image_)
    {
      auto& engine = core::Engine::get_singleton();

      vkDeviceWaitIdle(engine.get_device());

      vkDestroySampler(engine.get_device(), skybox_sampler_, nullptr);
      vkDestroyImageView(engine.get_device(), skybox_image_view_, nullptr);
      vkDestroyImage(engine.get_device(), skybox_image_, nullptr);
      vkFreeMemory(engine.get_device(), skybox_image_memory_, nullptr);
    }
  }

  void Scene::set_parent(Instance* parent)
  {
    throw std::runtime_error("cannot set the parent of a scene");
  }

  void Scene::load_skybox(const std::string& right, const std::string& left, const std::string& top,
                          const std::string& bottom, const std::string& front,
                          const std::string& back)
  {
    std::array<std::string, 6> faces = { right, left, top, bottom, front, back };
    std::array<stbi_uc*, 6> image_data;
    int width = 0;
    int height = 0;
    int channels = 0;

    for (int i = 0; i < 6; i++)
    {
      int w, h, c;
      image_data[i] = stbi_load(faces[i].c_str(), &w, &h, &c, STBI_rgb_alpha);
      if (!image_data[i])
        throw std::runtime_error("failed to load image");

      if (i == 0)
      {
        width = w;
        height = h;
        channels = 4;

        if (width != height)
          throw std::runtime_error("skybox images must have the same dimensions");
      }
      else if (w != width || h != height)
        throw std::runtime_error("skybox images must have the same dimensions");
    }

    auto& engine = core::Engine::get_singleton();
    VkDeviceSize image_size = width * height * 4;
    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;

    const VkBufferCreateInfo buffer_create_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = image_size * 6,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
    };

    auto flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    engine.create_buffer(buffer_create_info, flags, staging_buffer, staging_memory);

    void* data;
    VkResult result = vkMapMemory(engine.get_device(), staging_memory, 0, image_size * 6, 0, &data);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to map buffer memory");

    for (int i = 0; i < 6; i++)
      std::memcpy(static_cast<char*>(data) + (i * image_size), image_data[i], image_size);
    vkUnmapMemory(engine.get_device(), staging_memory);

    for (int i = 0; i < 6; i++)
      stbi_image_free(image_data[i]);

    const VkExtent3D image_extent = {
      .width = static_cast<uint32_t>(width),
      .height = static_cast<uint32_t>(height),
      .depth = 1,
    };

    const VkImageCreateInfo image_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_SRGB,
      .extent = image_extent,
      .mipLevels = 1,
      .arrayLayers = 6,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (skybox_image_)
    {
      vkDestroySampler(engine.get_device(), skybox_sampler_, nullptr);
      vkDestroyImageView(engine.get_device(), skybox_image_view_, nullptr);
      vkDestroyImage(engine.get_device(), skybox_image_, nullptr);
      vkFreeMemory(engine.get_device(), skybox_image_memory_, nullptr);
    }

    engine.create_image(image_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, skybox_image_,
                        skybox_image_memory_);

    const VkOffset3D offset = {
      .x = 0,
      .y = 0,
      .z = 0,
    };

    engine.transfer_image(skybox_image_, offset, image_extent, 6, staging_buffer);

    vkDestroyBuffer(engine.get_device(), staging_buffer, nullptr);
    vkFreeMemory(engine.get_device(), staging_memory, nullptr);

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
      .layerCount = 6,
    };

    const VkImageViewCreateInfo image_view_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = skybox_image_,
      .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
      .format = VK_FORMAT_R8G8B8A8_SRGB,
      .components = components,
      .subresourceRange = subresource_range,
    };

    result = vkCreateImageView(engine.get_device(), &image_view_create_info, nullptr,
                               &skybox_image_view_);
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
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 0.0f,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .minLod = 0.0f,
      .maxLod = 1.0f,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
    };

    result = vkCreateSampler(engine.get_device(), &sampler_create_info, nullptr, &skybox_sampler_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create sampler");
  }
} // namespace scene
