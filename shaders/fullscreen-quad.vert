#version 450 core
layout (location = 0) in vec3 vertPos;

layout (location = 0) out vec2 fragCoord;

void main(void)
{
  gl_Position = vec4(vertPos, 1.0);
  fragCoord = gl_Position.xy;
}
