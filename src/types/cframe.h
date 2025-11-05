#pragma once

#include <iostream>

#include "types/matrix4.h"
#include "types/vector3.h"

namespace types
{
  class CFrame
  {
  public:
    CFrame() = default;
    CFrame(const Vector3& pos);
    CFrame(const Vector3& pos, const Vector3& look_at);
    CFrame(const Vector3& pos, float r00, float r01, float r02, float r10, float r11, float r12,
           float r20, float r21, float r22);

    static CFrame from_axis_angle(const Vector3& axis, float angle);

    Matrix4 to_matrix() const;
    CFrame invert() const;

    CFrame operator+(const Vector3& vec) const;
    CFrame operator+=(const Vector3& vec);
    Vector3 operator*(const Vector3& vec) const;
    CFrame operator*(const CFrame& cf) const;
    CFrame operator*=(const CFrame& cf);

    Vector3 get_position() const;
    Vector3 get_right_vector() const;
    Vector3 get_up_vector() const;
    Vector3 get_look_vector() const;

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
