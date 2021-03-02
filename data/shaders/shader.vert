#version 460
#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec3 ivertex;
layout (location = 1) in vec3 inormal;
layout (location = 2) in vec2 iuvs;
layout (location = 3) in vec3 itangents;
layout (location = 4) in vec3 ibitangents;

layout (location = 0) out VertexOutput {
    vec3 normal;
    vec2 uvs;
};

layout (set = 0, binding = 0)
uniform Camera {
    mat4 projection;
    mat4 view;
};

layout (set = 0, binding = 1)
readonly buffer Transforms {
    mat4[] model;
};

layout (push_constant) uniform Constants {
    uint transform_index;
    uint texture_index;
};

void main() {
    gl_Position = projection * view * model[transform_index] * vec4(ivertex, 1.0);
    normal = inormal;
    uvs = iuvs;
}
