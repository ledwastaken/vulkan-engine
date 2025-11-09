#version 450 core
layout (location = 0) in vec3 normal;
layout (location = 1) in vec3 fragPos;
layout (location = 2) in vec3 viewPos;

layout (location = 0) out vec4 fragColor;

void main(void)
{
  // Render nothing, just write depth
}
