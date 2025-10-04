#version 450
layout(location = 0) in vec3 vertex_pos;
layout(location = 0) out vec3 texture_dir;

layout(push_constant) uniform Push {
    mat4 view;
    mat4 proj;
};

void main() {
  texture_dir = vertex_pos;
  gl_Position = proj * view * vec4(vertex_pos, 1.0);
}
