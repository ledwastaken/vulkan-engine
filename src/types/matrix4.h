#pragma once

#include <iostream>

namespace types
{
  class Matrix4
  {
  public:
    Matrix4() = default;
    Matrix4(const float data[16]);

    static Matrix4 identity();
    static Matrix4 frustum(float l, float r, float b, float t, float n, float f);
    static Matrix4 perpective(float fov, float ratio, float near, float far);

    const float* data() const;
    float* data();

  private:
    float data_[16] = {};
  };

  std::ostream& operator<<(std::ostream& out, const Matrix4& mat);
} // namespace types

#include "types/matrix4.hxx"
