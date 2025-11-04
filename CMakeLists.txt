cmake_minimum_required(VERSION 3.21.2)
project(vulkan-engine)

find_package(PkgConfig REQUIRED)
pkg_check_modules(VULKAN REQUIRED vulkan)
pkg_check_modules(SDL3 REQUIRED sdl3)
pkg_check_modules(STB REQUIRED stb)

add_executable(main
  src/core/asset-manager.cpp
  src/core/engine.cpp
  src/core/stb_impl.cpp

  src/gfx/pbr-pipeline.cpp
  src/gfx/pipeline.cpp
  src/gfx/skybox-pipeline.cpp

  src/render/deferred-renderer.cpp

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
)

target_include_directories(main PRIVATE
  src/
)

target_link_libraries(main PUBLIC
  ${VULKAN_LIBRARIES}
  ${SDL3_LIBRARIES}
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

# Find glslc compiler
find_program(GLSLC glslc HINTS 
    $ENV{VULKAN_SDK}/Bin
    $ENV{VULKAN_SDK}/bin
)

if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found!")
endif()

# Define your shader files
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

# List of shaders
set(SHADERS
    ${SHADER_SOURCE_DIR}/pbr/pbr.vert
    ${SHADER_SOURCE_DIR}/pbr/pbr.frag
)

# Compile each shader
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

# Create a custom target that depends on all shaders
add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})

# Make your executable depend on the shaders target
add_dependencies(main shaders)
