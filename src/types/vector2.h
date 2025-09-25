#pragma once

#include <iostream>

namespace types
{
  class Vector2
  {
  public:
    Vector2() = default;
    Vector2(float x, float y);

    Vector2 operator-() const;
    Vector2 operator+(const Vector2& vec) const;
    Vector2 operator-(const Vector2& vec) const;
    Vector2 operator*(float val) const;
    Vector2 operator/(float val) const;

    Vector2 operator+=(const Vector2& vec);
    Vector2 operator-=(const Vector2& vec);
    Vector2 operator*=(float val);
    Vector2 operator/=(float val);

    const float* data() const;
    float* data();

    float x;
    float y;
  };

  std::ostream& operator<<(std::ostream& out, const Vector2& vec);
} // namespace types

#include "types/vector2.hxx"
