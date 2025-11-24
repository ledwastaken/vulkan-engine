#version 450 core
layout (location = 0) in vec2 fragCoord;

layout (location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D depthSampler;

void main(void)
{
  float depthValue = texture(depthSampler, fragCoord.xy).r;

  fragColor = vec4(vec3(depthValue), 1.0);
}
