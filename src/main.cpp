#include <iostream>

#include "core/engine.hpp"

int main()
{
  core::Engine& engine = core::Engine::get();

  if (!engine.init())
    return 1;

  engine.run();

  std::cout << "Hello World!\n";

  return 0;
}
