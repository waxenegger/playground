#version 460

layout(binding = 0) uniform UniformBufferObject {
    mat4 viewproj;
    vec4 camera;
    vec4 lightColorAndGlossiness;
    vec4 lightLocationAndStrength;
} worldUniforms;

struct VertexData {
    float inPositionX, inPositionY, inPositionZ;
};

layout(binding = 1) readonly buffer verticesSSBO {
    VertexData vertices[];
};

layout(location = 0) out vec3 outPosition;

void main() {
    VertexData vertexData = vertices[gl_VertexIndex];
    vec3 inPosition = vec3(vertexData.inPositionX, vertexData.inPositionY, vertexData.inPositionZ);

    mat4 viewMatrix = worldUniforms.viewproj;
    viewMatrix[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);

    outPosition = inPosition;

    gl_Position = viewMatrix * vec4(inPosition.xyz, 1.0);
}

