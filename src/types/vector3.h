#pragma once

#include <iostream>

namespace types
{
  class Vector3
  {
  public:
    Vector3() = default;
    Vector3(float x, float y, float z);

    float magnitude() const;
    float dot(const Vector3& vec) const;
    Vector3 unit() const;
    Vector3 cross(const Vector3& vec) const;

    Vector3 operator-() const;
    Vector3 operator+(const Vector3& vec) const;
    Vector3 operator-(const Vector3& vec) const;
    Vector3 operator*(float val) const;
    Vector3 operator/(float val) const;

    Vector3 operator+=(const Vector3& vec);
    Vector3 operator-=(const Vector3& vec);
    Vector3 operator*=(float val);
    Vector3 operator/=(float val);

    const float* data() const;
    float* data();

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
  };

  std::ostream& operator<<(std::ostream& out, const Vector3& vec);
} // namespace types

#include "types/vector3.hxx"
