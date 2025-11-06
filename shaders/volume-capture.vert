#version 450

layout (location = 0) in vec3 vertPos;
layout (location = 1) in vec3 vertNormal;
layout (location = 2) in vec3 vertUV;

layout (location = 0) out vec3 fragPos;

layout (push_constant) uniform PushConstants {
    mat4 view;
    mat4 projection;
};

void main(void)
{
  gl_Position = projection * view * vec4(vertPos, 1.0);

  // TODO: Use normal and uv
  fragPos = vertPos;
}
