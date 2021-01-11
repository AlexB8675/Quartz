#version 460
#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) out vec4 fragment;

layout (location = 0) in VertexInput {
    vec3 color;
    vec2 uvs;
};

layout (set = 0, binding = 1) uniform sampler2D[] textures;

layout (push_constant) uniform Constants {
    uint texture_index;
};

void main() {
    fragment = vec4(texture(textures[texture_index], uvs).rgb, 1.0);
}
