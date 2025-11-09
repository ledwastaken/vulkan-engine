#version 450 core
layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexUV;

layout (location = 0) out vec3 normal;
layout (location = 1) out vec3 fragPos;
layout (location = 2) out vec3 viewPos;

layout(push_constant) uniform PushConstants {
  mat4 model;
  int sign;
};

layout (set = 0, binding = 0) uniform ubo {
  mat4 view;
  mat4 projection;
};

void main(void)
{
  vec4 worldPos = model * vec4(vertexPosition, 1.0);

  normal = mat3(model) * sign * vertexNormal;
  fragPos = worldPos.xyz;
  viewPos = -transpose(mat3(view)) * view[3].xyz;

  gl_Position = projection * view * worldPos;
}
