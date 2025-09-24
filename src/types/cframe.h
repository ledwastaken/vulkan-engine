#pragma once

#include <iostream>

#include "types/vector3.h"

namespace types
{
  class CFrame
  {
  public:
    CFrame() = default;
    CFrame(const Vector3& pos, const Vector3& look_at);

    const float* data() const;
    float* data();

  private:
    float r00_ = 1.0f;
    float r01_ = 0.0f;
    float r02_ = 0.0f;
    float r10_ = 0.0f;
    float r11_ = 1.0f;
    float r12_ = 0.0f;
    float r20_ = 0.0f;
    float r21_ = 0.0f;
    float r22_ = 1.0f;
    Vector3 pos_;
  };

  std::ostream& operator<<(std::ostream& out, const CFrame& cframe);
} // namespace types

#include "types/cframe.hxx"
