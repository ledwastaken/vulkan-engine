#version 450

layout (location = 0) in vec3 fragPos;
layout (location = 0) out vec4 fragCoord;

void main() {
    fragCoord = vec4(fragPos, 1.0);
}
