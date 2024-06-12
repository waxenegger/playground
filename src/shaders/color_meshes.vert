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
};

layout(binding = 1) readonly buffer verticesSSBO {
    VertexData vertices[];
};

layout(push_constant) uniform PushConstants {
    mat4 matrix;
    vec4 color;
} pushConstants;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormals;
layout(location = 2) out vec4 outColor;

void main() {
    VertexData vertexData = vertices[gl_VertexIndex];
    vec4 inPosition = vec4(vertexData.inPositionX, vertexData.inPositionY, vertexData.inPositionZ, 1.0f);

    gl_Position = worldUniforms.viewproj * pushConstants.matrix * inPosition;


    outPosition = (pushConstants.matrix * inPosition).xyz;
    outColor = pushConstants.color;

    outNormals = normalize(vec3(vertexData.inNormalX, vertexData.inNormalY, vertexData.inNormalZ));
}
