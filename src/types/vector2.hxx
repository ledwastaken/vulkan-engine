#include "types/vector2.h"

namespace types
{
  inline const float* Vector2::data() const { return &x; }
  inline float* Vector2::data() { return &x; }
} // namespace types
