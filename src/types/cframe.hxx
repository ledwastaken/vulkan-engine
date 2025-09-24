#include "types/cframe.h"

namespace types
{
  inline const float* CFrame::data() const { return &r00_; }
  inline float* CFrame::data() { return &r00_; }
} // namespace types
