#include "types/cframe.h"

#include <cmath>

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

  CFrame CFrame::from_axis_angle(const Vector3& axis, float angle)
  {
    float c = std::cos(angle);
    float s = std::sin(angle);
    float t = 1.0f - c;

    float x = axis.x / axis.magnitude();
    float y = axis.y / axis.magnitude();
    float z = axis.z / axis.magnitude();

    return CFrame(Vector3(), x * x * t + c, x * y * t + z * s, x * z * t - y * s, x * y * t - z * s,
                  y * y * t + c, y * z * t + x * s, x * z * t + y * s, y * z * t - x * s,
                  z * z * t + c);
  }

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

  CFrame CFrame::operator+=(const Vector3& vec)
  {
    pos_ += vec;

    return *this;
  }

  Vector3 CFrame::operator*(const Vector3& vec) const
  {
    float x = vec.x * r00_ + vec.y * r10_ + vec.z * r20_ + pos_.x;
    float y = vec.x * r01_ + vec.y * r11_ + vec.z * r21_ + pos_.y;
    float z = vec.x * r02_ + vec.y * r12_ + vec.z * r22_ + pos_.z;

    return Vector3(x, y, z);
  }

  CFrame CFrame::operator*(const CFrame& cf) const
  {
    auto res = CFrame(Vector3(), r00_ * cf.r00_ + r10_ * cf.r01_ + r20_ * cf.r02_,
                      r01_ * cf.r00_ + r11_ * cf.r01_ + r21_ * cf.r02_,
                      r02_ * cf.r00_ + r12_ * cf.r01_ + r22_ * cf.r02_,
                      r00_ * cf.r10_ + r10_ * cf.r11_ + r20_ * cf.r12_,
                      r01_ * cf.r10_ + r11_ * cf.r11_ + r21_ * cf.r12_,
                      r02_ * cf.r10_ + r12_ * cf.r11_ + r22_ * cf.r12_,
                      r00_ * cf.r20_ + r10_ * cf.r21_ + r20_ * cf.r22_,
                      r01_ * cf.r20_ + r11_ * cf.r21_ + r21_ * cf.r22_,
                      r02_ * cf.r20_ + r12_ * cf.r21_ + r22_ * cf.r22_);

    res.pos_ = Vector3(r00_ * cf.pos_.x + r10_ * cf.pos_.y + r20_ * cf.pos_.z + pos_.x,
                       r01_ * cf.pos_.x + r11_ * cf.pos_.y + r21_ * cf.pos_.z + pos_.y,
                       r02_ * cf.pos_.x + r12_ * cf.pos_.y + r22_ * cf.pos_.z + pos_.z);

    return res;
  }

  CFrame CFrame::operator*=(const CFrame& cf)
  {
    *this = operator*(cf);
    return *this;
  }

  Vector3 CFrame::get_position() const { return pos_; }
  Vector3 CFrame::get_right_vector() const { return Vector3(r00_, r01_, r02_); }
  Vector3 CFrame::get_up_vector() const { return Vector3(r10_, r11_, r12_); }
  Vector3 CFrame::get_look_vector() const { return -Vector3(r20_, r21_, r22_); }

  std::ostream& operator<<(std::ostream& out, const CFrame& cframe)
  {
    const float* data = cframe.data();

    return out << data[0] << ", " << data[1] << ", " << data[2] << ", " << data[3] << ", "
               << data[4] << ", " << data[5] << ", " << data[6] << ", " << data[7] << ", "
               << data[8] << ", " << data[9] << ", " << data[10] << ", " << data[11] << "\n";
  }
} // namespace types
