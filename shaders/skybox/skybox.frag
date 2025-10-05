#version 450
layout(location = 0) in vec3 texture_dir;
layout(location = 0) out vec4 frag_color;

void main() {
    vec3 dir = normalize(texture_dir);
    frag_color = vec4(0.5, 0.3, 0.3, 1.0);
}
