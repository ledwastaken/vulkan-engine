#include "gfx/skybox-pipeline.h"

namespace gfx
{
  inline void SkyboxPipeline::set_skybox_image(VkImage skybox_image)
  {
    skybox_image_ = skybox_image;
  }
} // namespace gfx
