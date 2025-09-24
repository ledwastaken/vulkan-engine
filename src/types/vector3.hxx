#include "types/vector3.h"

namespace types
{
  inline const float* Vector3::data() const { return &x; }
  inline float* Vector3::data() { return &x; }
} // namespace types
