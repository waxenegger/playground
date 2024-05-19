#version 440

layout(location = 0) in vec3 inPosition;

layout(binding = 2) uniform samplerCube skybox;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(skybox, inPosition);
}

