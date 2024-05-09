#version 460
#extension GL_EXT_debug_printf : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 viewproj;
    vec4 camera;
} worldUniforms;

struct VertexData {
    float inPositionX, inPositionY, inPositionZ;
    float inNormalX, inNormalY, inNormalZ;
    float r, g, b;
};

layout(binding = 1) readonly buffer verticesSSBO {
    VertexData vertices[];
};

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 color;

void main() {
    VertexData vertexData = vertices[gl_VertexIndex];
    vec4 inPosition = vec4(vertexData.inPositionX, vertexData.inPositionY, vertexData.inPositionZ, 1.0f);

    gl_Position = worldUniforms.viewproj * inPosition;
    fragPosition = vec3(inPosition);
    color = vec3(vertexData.r, vertexData.g, vertexData.b);
}
