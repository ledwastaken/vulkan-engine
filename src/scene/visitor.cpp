#include "scene/visitor.h"

#include "scene/camera.h"
#include "scene/mesh.h"
#include "scene/scene.h"

namespace scene
{
  void Visitor::operator()(Scene& scene)
  {
    for (auto child : scene.get_children())
      child->accept(*this);
  }

  void Visitor::operator()(Camera& camera)
  {
    for (auto child : camera.get_children())
      child->accept(*this);
  }

  void Visitor::operator()(Mesh& mesh)
  {
    for (auto child : mesh.get_children())
      child->accept(*this);
  }
} // namespace scene
