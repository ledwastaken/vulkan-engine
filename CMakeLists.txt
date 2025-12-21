cmake_minimum_required(VERSION 3.21.2)
project(vulkan-engine)

find_package(PkgConfig REQUIRED)
find_package(imgui REQUIRED)
find_program(GLSLC glslc)

pkg_check_modules(VULKAN REQUIRED vulkan)
pkg_check_modules(SDL3 REQUIRED sdl3)
pkg_check_modules(STB REQUIRED stb)

if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found!")
endif()

add_executable(main
  src/core/asset-manager.cpp
  src/core/engine.cpp

  src/gfx/csg-pipeline.cpp
  src/gfx/pipeline.cpp
  src/gfx/skybox-pipeline.cpp

  src/render/renderer.cpp

  src/scene/cube.cpp
  src/scene/instance.cpp
  src/scene/mesh.cpp
  src/scene/scene.cpp
  src/scene/visitor.cpp

  src/types/cframe.cpp
  src/types/matrix4.cpp
  src/types/vector2.cpp
  src/types/vector3.cpp

  src/main.cpp

  ${IMGUI_SOURCES}
)

target_compile_definitions(main PRIVATE STB_IMAGE_IMPLEMENTATION)
target_compile_definitions(main PRIVATE STB_IMAGE_STATIC)

target_include_directories(main PRIVATE
  src/
)

target_link_libraries(main PUBLIC
  ${VULKAN_LIBRARIES}
  ${SDL3_LIBRARIES}
  imgui
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

set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

set(SHADERS
  ${SHADER_SOURCE_DIR}/csg-diff-frontface.frag
  ${SHADER_SOURCE_DIR}/csg-diff.vert
  ${SHADER_SOURCE_DIR}/csg.frag
  ${SHADER_SOURCE_DIR}/csg.vert
  ${SHADER_SOURCE_DIR}/depth-display.frag
  ${SHADER_SOURCE_DIR}/depth.frag
  ${SHADER_SOURCE_DIR}/fullscreen-quad.vert
  ${SHADER_SOURCE_DIR}/skybox.vert
  ${SHADER_SOURCE_DIR}/skybox.frag
)

foreach(SHADER ${SHADERS})
  get_filename_component(SHADER_NAME ${SHADER} NAME)
  set(SHADER_OUTPUT ${SHADER_BINARY_DIR}/${SHADER_NAME}.spv)

  add_custom_command(
    OUTPUT ${SHADER_OUTPUT}
    COMMAND ${GLSLC} ${SHADER} -o ${SHADER_OUTPUT}
    DEPENDS ${SHADER}
    COMMENT "Compiling ${SHADER_NAME}"
  )

  list(APPEND SPIRV_BINARY_FILES ${SHADER_OUTPUT})
endforeach()

add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})

add_dependencies(main shaders)

target_compile_definitions(main PRIVATE 
  SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders/"
)
