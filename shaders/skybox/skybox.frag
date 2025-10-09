#version 450
layout(location = 0) in vec3 texture_dir;
layout(location = 0) out vec4 frag_color;

layout(set = 0, binding = 0) uniform samplerCube skybox_sampler;

void main() {
    vec3 dir = normalize(texture_dir);
    vec3 color = texture(skybox_sampler, dir).rgb;
    frag_color = vec4(color, 1.0);
}
