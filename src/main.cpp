#include <iostream>
#include <stdexcept>

#include "core/engine.h"
#include "core/scene-manager.h"
#include "scene/mesh.h"

using namespace core;
using namespace scene;

void init(Scene& scene)
{
  auto mesh = new Mesh();

  std::vector<Vertex> vertices = {
    {
        .position = { -0.9f, 0.9f, 0.0f },
        .normal = { 0.0f, 0.0f, 1.0f },
        .uv = { 0.0f, 0.0f },
    },
    {
        .position = { -0.9f, -0.9f, 0.0f },
        .normal = { 0.0f, 0.0f, 1.0f },
        .uv = { 0.0f, 0.0f },
    },
    {
        .position = { 0.9f, -0.9f, 0.0f },
        .normal = { 0.0f, 0.0f, 1.0f },
        .uv = { 0.0f, 0.0f },
    },
    {
        .position = { 0.9f, -0.9f, 0.0f },
        .normal = { 0.0f, 0.0f, 1.0f },
        .uv = { 0.0f, 0.0f },
    },
    {
        .position = { 0.9f, 0.9f, 0.0f },
        .normal = { 0.0f, 0.0f, 1.0f },
        .uv = { 0.0f, 0.0f },
    },
    {
        .position = { -0.9f, 0.9f, 0.0f },
        .normal = { 0.0f, 0.0f, 1.0f },
        .uv = { 0.0f, 0.0f },
    },
  };

  mesh->load_vertices(vertices);
  mesh->set_parent(&scene);
}

int main(int argc, char* argv[])
{
  try
  {
    auto& engine = Engine::get_singleton();
    auto& scene_manager = SceneManager::get_singleton();
    auto scene = new Scene();

    engine.init(argc, argv);
    init(*scene);
    scene_manager.set_current_scene(scene);
    engine.loop();
    delete scene;
    engine.quit();

    return 0;
  }

  // Required to enable stack unwinding.
  catch (const std::invalid_argument& e)
  {
    std::cerr << "invalid argument: " << e.what() << '\n';
    return 64;
  }
  catch (const std::runtime_error& e)
  {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }
}
