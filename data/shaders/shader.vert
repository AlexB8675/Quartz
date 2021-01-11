#version 460

layout (location = 0) in vec3 ivertex;
layout (location = 1) in vec3 icolor;
layout (location = 2) in vec2 iuvs;

layout (location = 0) out VertexOutput {
    vec3 color;
    vec2 uvs;
};

layout (set = 0, binding = 0)
uniform Camera {
    mat4 projection;
    mat4 view;
};

layout (push_constant) uniform Constants {
    uint texture_index;
};

void main() {
    gl_Position = projection * view * vec4(ivertex, 1.0);
    color = icolor;
    uvs = iuvs;
}
