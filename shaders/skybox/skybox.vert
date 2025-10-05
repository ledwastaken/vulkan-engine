#version 450
layout(location = 0) in vec3 vertex_pos;
layout(location = 0) out vec3 texture_dir;

layout(push_constant) uniform push_constants {
    mat3 view;
    mat4 projection;
};

void main() {
  texture_dir = vertex_pos;
  gl_Position = projection * view * vec4(vertex_pos, 1.0);
}
