#include "types/vector3.h"

#include <cmath>

namespace types
{
  Vector3::Vector3(float x, float y, float z)
    : x(x)
    , y(y)
    , z(z)
  {}

  float Vector3::magnitude() const { return std::sqrt(x * x + y * y + z * z); }
  float Vector3::dot(const Vector3& vec) const { return x * vec.x + y * vec.y + z * vec.z; }
  Vector3 Vector3::unit() const { return *this / magnitude(); }

  Vector3 Vector3::cross(const Vector3& vec) const
  {
    return Vector3(y * vec.z - z * vec.y, z * vec.x - x * vec.z, x * vec.y - y * vec.x);
  }

  Vector3 Vector3::operator-() const { return Vector3(-x, -y, -z); }
  Vector3 Vector3::operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
  Vector3 Vector3::operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
  Vector3 Vector3::operator*(float val) const { return Vector3(x * val, y * val, z * val); }
  Vector3 Vector3::operator/(float val) const { return Vector3(x / val, y / val, z / val); }

  Vector3 Vector3::operator+=(const Vector3& vec)
  {
    x += vec.x;
    y += vec.y;
    z += vec.z;

    return *this;
  }

  Vector3 Vector3::operator-=(const Vector3& vec)
  {
    x -= vec.x;
    y -= vec.y;
    z -= vec.z;

    return *this;
  }

  Vector3 Vector3::operator*=(float val)
  {
    x *= val;
    y *= val;
    z *= val;

    return *this;
  }

  Vector3 Vector3::operator/=(float val)
  {
    x /= val;
    y /= val;
    z /= val;

    return *this;
  }

  std::ostream& operator<<(std::ostream& out, const Vector3& vec)
  {
    return out << vec.x << ", " << vec.y << ", " << vec.z;
  }
} // namespace types
