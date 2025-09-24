#include "scene/instance.h"

namespace scene
{
  Instance::Instance(const std::string& name)
    : name(name)
  {}

  Instance::~Instance()
  {
    for (auto child : children_)
      delete child;
  }

  void Instance::set_parent(Instance* parent)
  {
    if (parent_)
      parent_->children_.erase(this);
    if (parent)
      parent->children_.insert(this);
    parent_ = parent;
  }

  Instance* Instance::find_first_child(const std::string& name) const
  {
    for (auto child : children_)
      if (child->name == name)
        return child;
    return nullptr;
  }
} // namespace scene
