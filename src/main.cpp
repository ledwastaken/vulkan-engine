#include <iostream>

#include "core/engine.h"

using namespace core;

int main()
{
  Engine& engine = Engine::get();

  if (!engine.init())
    return 1;

  engine.run();

  std::cout << "Hello World!\n";

  return 0;
}
