#include "types/matrix4.h"

namespace types
{
  inline const float* Matrix4::data() const { return data_; }
  inline float* Matrix4::data() { return data_; }
} // namespace types
