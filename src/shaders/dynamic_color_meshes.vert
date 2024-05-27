#version 460
#extension GL_EXT_debug_printf : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 viewproj;
    vec4 camera;
    vec4 lightColorAndGlossiness;
    vec4 lightLocationAndStrength;
} worldUniforms;

struct VertexData {
    float inPositionX, inPositionY, inPositionZ;
    float inNormalX, inNormalY, inNormalZ;
    float inColorRed, inColorGreen, inColorBlue;
};

layout(binding = 1) readonly buffer verticesSSBO {
    VertexData vertices[];
};

layout(push_constant) uniform PushConstants {
    mat4 matrix;
} pushConstants;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormals;
layout(location = 2) out vec3 outColor;


void main() {
    VertexData vertexData = vertices[gl_VertexIndex];
    vec4 inPosition = vec4(vertexData.inPositionX, vertexData.inPositionY, vertexData.inPositionZ, 1.0f);

    gl_Position = worldUniforms.viewproj * pushConstants.matrix * inPosition;

    outPosition = inPosition.xyz;
    outColor = vec3(vertexData.inColorRed, vertexData.inColorGreen, vertexData.inColorBlue);

    vec4 transformedNormals = pushConstants.matrix * vec4(vertexData.inNormalX, vertexData.inNormalY, vertexData.inNormalZ, 1.0f);
    outNormals = normalize(transformedNormals.xyz);
}
