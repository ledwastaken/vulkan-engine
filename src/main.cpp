#include <iostream>
#include <stdexcept>

#include "core/engine.h"

using namespace core;

int main(int argc, char* argv[])
{
  try
  {
    Engine& engine = Engine::get_singleton();

    engine.init(argc, argv);

    // FIXME: init scene

    engine.loop();
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
