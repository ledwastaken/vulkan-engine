#version 450

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec2 fragUV;

layout (location = 0) out vec4 texPos;

void main() {
    texPos = vec4(fragPos, 1.0);

    // TODO: normal and uv
}
