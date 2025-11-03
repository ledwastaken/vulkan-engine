#include "types/cframe.h"

namespace types
{
  CFrame::CFrame(const Vector3& pos)
    : pos_(pos)
  {}

  CFrame::CFrame(const Vector3& pos, const Vector3& look_at)
    : pos_(pos)
  {
    auto f = (look_at - pos).unit();

    auto up = Vector3(0, 1, 0);

    auto s = f.cross(up).unit();
    auto u = s.cross(f);

    r00_ = s.x;
    r01_ = s.y;
    r02_ = s.z;
    r10_ = u.x;
    r11_ = u.y;
    r12_ = u.z;
    r20_ = -f.x;
    r21_ = -f.y;
    r22_ = -f.z;
  }

  CFrame::CFrame(const Vector3& pos, float r00, float r01, float r02, float r10, float r11,
                 float r12, float r20, float r21, float r22)
    : pos_(pos)
    , r00_(r00)
    , r01_(r01)
    , r02_(r02)
    , r10_(r10)
    , r11_(r11)
    , r12_(r12)
    , r20_(r20)
    , r21_(r21)
    , r22_(r22)
  {}

  Matrix4 CFrame::to_matrix() const
  {
    // clang-format off
    const float data[] = {
      r00_,   r01_,   r02_,   0.0f,
      r10_,   r11_,   r12_,   0.0f,
      r20_,   r21_,   r22_,   0.0f,
      pos_.x, pos_.y, pos_.z, 1.0f,
    };
    // clang-format on

    return Matrix4(data);
  }

  CFrame CFrame::invert() const
  {
    CFrame cf(Vector3(), r00_, r10_, r20_, r01_, r11_, r21_, r02_, r12_, r22_);

    cf.pos_ = (cf * -pos_);

    return cf;
  }

  CFrame CFrame::operator+(const Vector3& vec) const
  {
    return CFrame(pos_ + vec, r00_, r01_, r02_, r10_, r11_, r12_, r20_, r21_, r22_);
  }

  Vector3 CFrame::operator*(const Vector3& vec) const
  {
    float x = vec.x * r00_ + vec.y * r10_ + vec.z * r20_ + pos_.x;
    float y = vec.x * r01_ + vec.y * r11_ + vec.z * r21_ + pos_.y;
    float z = vec.x * r02_ + vec.y * r12_ + vec.z * r22_ + pos_.z;

    return Vector3(x, y, z);
  }

  std::ostream& operator<<(std::ostream& out, const CFrame& cframe)
  {
    const float* data = cframe.data();

    return out << data[0] << ", " << data[1] << ", " << data[2] << ", " << data[3] << ", "
               << data[4] << ", " << data[5] << ", " << data[6] << ", " << data[7] << ", "
               << data[8] << ", " << data[9] << ", " << data[10] << ", " << data[11] << "\n";
  }
} // namespace types
