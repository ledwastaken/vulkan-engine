#version 450 core
layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexUV;

layout (set = 0, binding = 0) uniform mat4 model;
layout (set = 0, binding = 1) uniform mat4 view;
layout (set = 0, binding = 2) uniform mat4 proj;

void main(void)
{
  gl_Position = model * vec4(vertexPosition, 1.0);
}
