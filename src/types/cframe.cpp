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

  std::ostream& operator<<(std::ostream& out, const CFrame& cframe)
  {
    const float* data = cframe.data();

    return out << data[0] << ", " << data[1] << ", " << data[2] << ", " << data[3] << ", "
               << data[4] << ", " << data[5] << ", " << data[6] << ", " << data[7] << ", "
               << data[8] << ", " << data[9] << ", " << data[10] << ", " << data[11] << "\n";
  }
} // namespace types
