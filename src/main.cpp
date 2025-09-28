#include <iostream>
#include <stdexcept>

#include "core/engine.h"
#include "core/scene-manager.h"

using namespace core;
using namespace scene;

void init(Scene& scene)
{
  // FIXME
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
