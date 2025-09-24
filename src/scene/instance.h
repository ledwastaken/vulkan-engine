#pragma once

#include <set>
#include <string>

namespace scene
{
  class Instance
  {
  public:
    Instance(Instance&) = delete;
    Instance operator=(Instance&) = delete;
    Instance() = default;
    Instance(const std::string& name);

    virtual ~Instance();

    virtual void set_parent(Instance* parent);
    virtual Instance* get_parent() const;

    virtual std::set<Instance*> get_children() const;
    virtual Instance* find_first_child(const std::string& name) const;

  public:
    std::string name;

  protected:
    Instance* parent_ = nullptr;
    std::set<Instance*> children_;
  };
} // namespace scene

#include "scene/instance.hxx"
