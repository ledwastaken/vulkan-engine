#version 450 core
layout (location = 0) in vec3 normal;
layout (location = 1) in vec3 fragPos;
layout (location = 2) in vec3 viewPos;

layout(set = 1, binding = 0) uniform sampler2D rayEnter;
layout(set = 1, binding = 1) uniform sampler2D rayExit;

layout (location = 0) out vec4 fragColor;

void main(void)
{
  vec3 lightPos   = vec3(-10.0, 20.0, -4.0);
  vec3 lightColor = vec3(1.0, 1.0, 1.0);
  vec3 albedo     = vec3(0.9, 0.1, 0.1);

  vec3 lightDir   = normalize(lightPos - fragPos);
  vec3 viewDir    = normalize(viewPos - fragPos);

  vec3 ambient = vec3(0.1, 0.1, 0.1) * albedo;

  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = diff * lightColor * albedo;

  vec3 color = ambient + diffuse;

  fragColor = vec4(color, 1.0);
}
