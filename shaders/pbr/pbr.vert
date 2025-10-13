#version 450 core
layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexUV;

layout (set = 0, binding = 0) uniform ubo {
  mat4 model;
  mat4 view;
  mat4 projection;
};

void main(void)
{
  gl_Position = projection * view * model * vec4(vertexPosition, 1.0);
}
