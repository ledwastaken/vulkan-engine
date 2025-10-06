#pragma once

#include <memory>
#include <unordered_map>

#include <vulkan/vulkan.h>

#include "misc/singleton.h"

namespace core
{
  struct ImageData
  {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView image_view;
    VkSampler sampler;
  };

  class AssetManager : public misc::Singleton<AssetManager>
  {
    // Give Singleton<AssetManager> access to AssetManager’s private constructor
    friend class Singleton<AssetManager>;

  private:
    AssetManager() = default;

  public:
    ImageData* load_image(const std::string& path, const std::string& name);
    ImageData* find_image(const std::string& name);
    void destroy_image(const std::string& name);

    void free();

  private:
    std::unordered_map<std::string, ImageData*> images_;
  };
} // namespace core
