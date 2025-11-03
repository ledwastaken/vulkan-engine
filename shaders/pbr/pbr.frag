#version 450 core
layout (location = 0) in vec3 normal;
layout (location = 1) in vec3 fragPos;
layout (location = 2) in vec3 viewPos;

layout (location = 0) out vec4 fragColor;

void main(void)
{
  vec3 lightPos   = vec3(10.0, 10.0, 10.0);
  vec3 lightColor = vec3(1.0, 1.0, 1.0);

  vec3 lightDir   = normalize(lightPos - fragPos);
  vec3 viewDir    = normalize(viewPos - fragPos);

  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = diff * lightColor;

  vec3 color = diffuse;

  fragColor = vec4(color, 1.0);
}
