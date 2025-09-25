#include "types/vector2.h"

namespace types
{
  Vector2::Vector2(float x, float y)
    : x(x)
    , y(y)
  {}

  Vector2 Vector2::operator-() const { return Vector2(-x, -y); }
  Vector2 Vector2::operator+(const Vector2& vec) const { return Vector2(x + vec.x, y + vec.y); }
  Vector2 Vector2::operator-(const Vector2& vec) const { return Vector2(x - vec.x, y - vec.y); }
  Vector2 Vector2::operator*(float val) const { return Vector2(x * val, y * val); }
  Vector2 Vector2::operator/(float val) const { return Vector2(x / val, y / val); }

  Vector2 Vector2::operator+=(const Vector2& vec)
  {
    x += vec.x;
    y += vec.y;

    return *this;
  }

  Vector2 Vector2::operator-=(const Vector2& vec)
  {
    x -= vec.x;
    y -= vec.y;

    return *this;
  }

  Vector2 Vector2::operator*=(float val)
  {
    x *= val;
    y *= val;

    return *this;
  }

  Vector2 Vector2::operator/=(float val)
  {
    x /= val;
    y /= val;

    return *this;
  }

  std::ostream& operator<<(std::ostream& out, const Vector2& vec)
  {
    return out << vec.x << ", " << vec.y;
  }
} // namespace types
