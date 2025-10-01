cmake_minimum_required(VERSION 3.21.2)
project(vulkan-engine)

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED)

add_executable(main
  src/core/engine.cpp

  src/render/renderer.cpp

  src/scene/instance.cpp
  src/scene/mesh.cpp
  src/scene/scene.cpp
  src/scene/visitor.cpp

  src/types/cframe.cpp
  src/types/matrix4.cpp
  src/types/vector2.cpp
  src/types/vector3.cpp

  src/main.cpp
)

target_include_directories(main PRIVATE
  src/
)

target_link_libraries(main PUBLIC
  Vulkan::Vulkan
  SDL3::SDL3
)

target_compile_options(main PRIVATE
  -Wall -Wextra -Wold-style-cast -pedantic -std=c++20
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(main PRIVATE
    -g -fsanitize=leak
  )

  target_link_options(main PRIVATE
    -fsanitize=leak
  )
endif()
