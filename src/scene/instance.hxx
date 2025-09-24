#include "scene/instance.h"

namespace scene
{
  inline Instance* Instance::get_parent() const { return parent_; }
  inline std::set<Instance*> Instance::get_children() const { return children_; }
} // namespace scene
