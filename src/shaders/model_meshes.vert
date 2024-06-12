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
    float inUvX, inUvY;
    float inTangentX, inTangentY, inTangentZ;
    float inBiTangentX, inBiTangentY, inBiTangentZ;
};

layout(binding = 1) readonly buffer verticesSSBO {
    VertexData vertices[];
};

layout(push_constant) uniform PushConstants {
    mat4 matrix;
    vec4 color;
    vec4 specularColor;
    int ambientTexture;
    int diffuseTexture;
    int specularTexture;
    int normalTexture;
} pushConstants;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormals;
layout(location = 2) out vec2 outUV;

void main() {
    VertexData vertexData = vertices[gl_VertexIndex];
    vec4 inPosition = vec4(vertexData.inPositionX, vertexData.inPositionY, vertexData.inPositionZ, 1.0f);
    vec3 inTangent = vec3(vertexData.inTangentX, vertexData.inTangentY, vertexData.inTangentZ);
    vec3 inBitangent = vec3(vertexData.inBiTangentX, vertexData.inBiTangentY, vertexData.inBiTangentZ);

    gl_Position = worldUniforms.viewproj * pushConstants.matrix * inPosition;

    outPosition = (pushConstants.matrix * inPosition).xyz;
    outUV = vec2(vertexData.inUvX, vertexData.inUvY);

    outNormals = normalize(vec3(vertexData.inNormalX, vertexData.inNormalY, vertexData.inNormalZ));

    // TODO: normal map
    /*
    if (pushConstants.normalTexture != -1) {
        vec3 T = normalize(pushConstants.matrix * inTangent);
        vec3 N = normalize(pushConstants.matrix * inNormal);
        vec3 B = normalize(pushConstants.matrix * inBitangent);

        T = normalize(T - dot(T, N) * N);
        if (dot(cross(N, T), B) < 0.0f) T *= -1.0f;
        mat3 TBN = transpose(mat3(T, cross(N, T), N));

        fragPosition = TBN * fragPosition;
        eye = vec4(TBN * vec3(eye), 1.0f);
        light = vec4(TBN * vec3(light), 1.0f);
    }*/
}
