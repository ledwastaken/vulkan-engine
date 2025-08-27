cmake_minimum_required(VERSION 3.21.2)
project(vulkan-engine)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

add_executable(main
  src/main.cpp
)

target_link_libraries(main PUBLIC
  Vulkan::Vulkan
  SDL2::SDL2
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
