#pragma once

#include "scene/fwd.h"

namespace scene
{
  class Visitor
  {
  public:
    virtual void operator()(Scene& scene);
    virtual void operator()(Camera& camera);
    virtual void operator()(Mesh& mesh);
  };
} // namespace scene
