#version 460
#extension GL_EXT_debug_printf : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 viewproj;
    vec4 camera;
} worldUniforms;

struct VertexData {
    float inPositionX, inPositionY, inPositionZ;
    float inNormalX, inNormalY, inNormalZ;
    float inColorRed, inColorGreen, inColorBlue;
};

layout(binding = 1) readonly buffer verticesSSBO {
    VertexData vertices[];
};

layout(location = 0) out vec3 outColor;
//layout(location = 1) out vec3 outNormals;


void main() {
    VertexData vertexData = vertices[gl_VertexIndex];
    vec4 inPosition = worldUniforms.viewproj * vec4(vertexData.inPositionX, vertexData.inPositionY, vertexData.inPositionZ, 1.0f);

    gl_Position = inPosition;
    outColor = vec3(vertexData.inColorRed, vertexData.inColorGreen, vertexData.inColorBlue);
    //outNormals = vec3(vertexData.inNormalX, vertexData.inNormalY, vertexData.inNormalZ);

}
