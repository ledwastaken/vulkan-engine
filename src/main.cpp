#include <iostream>
#include <memory>
#include <stdexcept>

#include "core/asset-manager.h"
#include "core/engine.h"
#include "core/scene-manager.h"
#include "gfx/skybox-pipeline.h"
#include "scene/mesh.h"

using namespace core;
using namespace gfx;
using namespace scene;
using namespace types;

static const std::vector<Vertex> vertices = {
  {
      .position = { -1.0f, 1.0f, 0.0f },
      .normal = { 0.0f, 0.0f, 1.0f },
      .uv = { 0.0f, 0.0f },
  },
  {
      .position = { -1.0f, -1.0f, 0.0f },
      .normal = { 0.0f, 0.0f, 1.0f },
      .uv = { 0.0f, 0.0f },
  },
  {
      .position = { 1.0f, -1.0f, 0.0f },
      .normal = { 0.0f, 0.0f, 1.0f },
      .uv = { 0.0f, 0.0f },
  },
  {
      .position = { 1.0f, 1.0f, 0.0f },
      .normal = { 0.0f, 0.0f, 1.0f },
      .uv = { 0.0f, 0.0f },
  },
};

static const std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

void init(Scene& scene)
{
  scene.load_skybox("assets/texture/grid-texture.jpg", "assets/texture/grid-texture.jpg",
                    "assets/texture/grid-texture.jpg", "assets/texture/grid-texture.jpg",
                    "assets/texture/grid-texture.jpg", "assets/texture/grid-texture.jpg");

  auto mesh = new Mesh();
  mesh->load_mesh_data(vertices, indices);
  mesh->set_parent(&scene);

  auto camera = new Camera();
  camera->cframe = CFrame(Vector3(0, 0, 2), Vector3());
  camera->set_parent(&scene);

  scene.current_camera = camera;
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
