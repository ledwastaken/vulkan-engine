#version 450

layout (location = 0) in vec3 vertPos;
layout (location = 1) in vec3 vertNormal;
layout (location = 2) in vec2 vertUV;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec2 fragUV;

layout (set = 0, binding = 0) uniform UBO {
  mat4 model;
  mat4 view;
  mat4 projection;
};

void main(void)
{
  vec4 worldPos = model * vec4(vertPos, 1.0);
  gl_Position = projection * view * worldPos;

  fragPos = worldPos.xyz;
  fragNormal = mat3(model) * vertNormal;
  fragUV = vertUV;
}
