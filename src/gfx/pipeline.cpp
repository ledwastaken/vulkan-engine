#include "gfx/pipeline.h"

#include <fstream>
#include <vector>

#include "core/engine.h"

namespace gfx
{
  void Pipeline::create_shader_module(const char* path, VkShaderModule* shader_module)
  {
    auto& engine = core::Engine::get_singleton();

    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
      throw std::runtime_error("failed to open file");

    size_t file_size = file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    const VkShaderModuleCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .codeSize = file_size,
      .pCode = reinterpret_cast<uint32_t*>(buffer.data()),
    };

    VkResult result =
        vkCreateShaderModule(engine.get_device(), &create_info, nullptr, shader_module);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create shader module");
  }
} // namespace gfx
