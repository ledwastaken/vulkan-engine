#pragma once

#include "scene/instance.h"
#include "types/cframe.h"

namespace scene
{
  class Object : public Instance
  {
  public:
    Object() = default;

    types::CFrame cframe;
  };
} // namespace scene
