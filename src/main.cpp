#include <iostream>
#include <memory>
#include <stdexcept>

#include "core/asset-manager.h"
#include "core/engine.h"
#include "core/scene-manager.h"
#include "gfx/skybox-pipeline.h"
#include "scene/cube.h"

using namespace core;
using namespace gfx;
using namespace scene;
using namespace types;

void init(Scene& scene)
{
  scene.load_skybox("assets/texture/skybox/right.jpg", "assets/texture/skybox/left.jpg",
                    "assets/texture/skybox/top.jpg", "assets/texture/skybox/bottom.jpg",
                    "assets/texture/skybox/front.jpg", "assets/texture/skybox/back.jpg");

  auto mesh = new Mesh();
  mesh->load_mesh_from_file("assets/geometry/cube.obj");
  mesh->cframe = CFrame(Vector3(0, 0, 0));

  auto substractive_mesh = new Mesh();
  substractive_mesh->load_mesh_from_file("assets/geometry/sphere.obj");
  substractive_mesh->cframe = CFrame(Vector3(0, 0, 0));

  auto camera = new Camera();
  camera->cframe = CFrame(Vector3(-8, 4, -4), Vector3(0, 0, 0));
  camera->set_parent(&scene);

  scene.current_camera = camera;
  scene.mesh = mesh;
  scene.substractive_mesh = substractive_mesh;
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
