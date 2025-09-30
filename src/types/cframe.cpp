#include "types/cframe.h"

namespace types
{
  CFrame::CFrame(const Vector3& pos, const Vector3& look_at)
    : pos_(pos)
  {
    auto z_dir = (look_at - pos).unit();
    auto x_dir = z_dir.cross(Vector3(0, 1, 0));
    auto y_dir = x_dir.cross(z_dir);

    r00_ = x_dir.x;
    r01_ = y_dir.x;
    r02_ = -z_dir.x;
    r10_ = x_dir.y;
    r11_ = y_dir.y;
    r12_ = -z_dir.y;
    r20_ = x_dir.z;
    r21_ = y_dir.z;
    r22_ = -z_dir.z;
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

  CFrame CFrame::invert() const
  {
    CFrame cf(Vector3(), r00_, r10_, r20_, r01_, r11_, r21_, r02_, r12_, r22_);

    cf.pos_ = (cf * -pos_);

    return cf;
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
