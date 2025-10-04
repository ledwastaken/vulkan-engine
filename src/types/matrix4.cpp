#include "types/matrix4.h"

#include <cmath>
#include <cstring>

namespace types
{
  Matrix4::Matrix4(const float data[16]) { std::memcpy(data_, data, sizeof(float) * 16); }

  Matrix4 Matrix4::identity()
  {
    // clang-format off
    const float data[16] = {
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1,
    };
    // clang-format on

    return Matrix4(data);
  }

  Matrix4 Matrix4::frustum(float l, float r, float b, float t, float n, float f)
  {
    float v0 = (2 * n);
    float v1 = (r - l);
    float v2 = (t - b);
    float v3 = (f - n);

    // clang-format off
    float data[16] = {
        v0 / v1,        0,              0,               0,
        0,              -v0 / v2,       0,               0,
        (r + l) / v1,   (t + b) / v2,   -(f + n) / v3,   -1,
        0,              0,              -v0 * f / v3,    0
    };
    // clang-format on

    return Matrix4(data);
  }

  Matrix4 Matrix4::perpective(float fov, float ratio, float near, float far)
  {
    float top = std::tan(fov * 0.5f) * near;
    float bottom = -top;
    float right = top * ratio;
    float left = -right;

    return frustum(left, right, bottom, top, near, far);
  }

  std::ostream& operator<<(std::ostream& out, const Matrix4& mat)
  {
    const float* data = mat.data();

    return out << data[0] << ", " << data[4] << ", " << data[8] << ", " << data[12] << "\n"
               << data[1] << ", " << data[5] << ", " << data[9] << ", " << data[13] << "\n"
               << data[2] << ", " << data[6] << ", " << data[10] << ", " << data[14] << "\n"
               << data[3] << ", " << data[7] << ", " << data[11] << ", " << data[15];
  }
} // namespace types
