#include <iostream>

#include "core/engine.h"

using namespace core;

int main()
{
  Engine& engine = Engine::singleton();

  try
  {
    engine.init();
    engine.quit();    

    return 0;
  }
  catch(const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }
}
