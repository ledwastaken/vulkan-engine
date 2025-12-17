#version 450 core
layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 viewPos;

layout(set = 1, binding = 0) uniform sampler2D rayEnter;
layout(set = 1, binding = 1) uniform sampler2D rayLeave;
layout(set = 1, binding = 2) uniform sampler2D frontDepth;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out float fragMask; 

float linearDepth(float z, float near, float far) {
  return (2.0 * near) / (far + near - z * (far - near));
}

void main(void)
{
  vec2 uv = gl_FragCoord.xy / vec2(800.0, 600.0);
  float rayEnterDepth = texture(rayEnter, uv).r;
  float rayLeaveDepth = texture(rayLeave, uv).r;
  float frontDepthValue = texture(frontDepth, uv).r;

  if (rayLeaveDepth < 1.0 && rayEnterDepth == 1.0)
    rayEnterDepth = 0.0;

  if (gl_FragCoord.z < 1.0 && frontDepthValue == 1.0)
    frontDepthValue = 0.0;

  if (gl_FragCoord.z < rayEnterDepth || frontDepthValue > rayEnterDepth)
    discard;

  if (gl_FragCoord.z > rayLeaveDepth)
    discard;

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
  fragMask = 1;
}
