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
};

layout(binding = 1) readonly buffer verticesSSBO {
    VertexData vertices[];
};

struct IndirectDrawCommand {
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
    uint meshInstance;
};

layout(binding = 2) buffer indirectDrawSSBO {
    IndirectDrawCommand draws[];
};

struct InstanceData {
    mat4 matrix;
    float centerX;
    float centerY;
    float centerZ;
    float radius;
};

layout(binding = 3) buffer instanceSSBO {
    InstanceData instances[];
};

struct MeshData {
    uint textureId;
};

layout(binding = 4) buffer meshDataSSBO {
    MeshData meshes[];
};

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormals;
layout(location = 2) out vec2 outUV;
layout(location = 3) out uint outTextureId;

void main() {
    VertexData vertexData = vertices[gl_VertexIndex];
    IndirectDrawCommand draw = draws[gl_DrawID];
    InstanceData instanceData = instances[draw.firstInstance];
    MeshData meshData = meshes[draw.meshInstance];

    vec4 inPosition = vec4(vertexData.inPositionX, vertexData.inPositionY, vertexData.inPositionZ, 1.0f);
    gl_Position = worldUniforms.viewproj * instanceData.matrix * inPosition;

    outPosition = inPosition.xyz;
    outUV = vec2(vertexData.inUvX, vertexData.inUvY);
    outTextureId = meshData.textureId;

    outNormals = normalize(vec3(vertexData.inNormalX, vertexData.inNormalY, vertexData.inNormalZ));
}
