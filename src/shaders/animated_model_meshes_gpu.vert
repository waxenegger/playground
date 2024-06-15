#version 460

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
    vec4 color;
    vec4 specularColor;
    int ambientTexture;
    int diffuseTexture;
    int specularTexture;
    int normalTexture;
};

layout(binding = 4) buffer meshDataSSBO {
    MeshData meshes[];
};

layout(binding = 6) readonly buffer animationMatricesSSBO {
    mat4 matrices[];
};

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormals;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec3 outCameraTBN;
layout(location = 4) out vec3 outLightTBN;
layout(location = 5) flat out MeshData outMeshData;


void main() {
    VertexData vertexData = vertices[gl_VertexIndex];
    IndirectDrawCommand draw = draws[gl_DrawID];
    InstanceData instanceData = instances[draw.firstInstance];
    MeshData meshData = meshes[draw.meshInstance];

    vec4 inPosition = vec4(vertexData.inPositionX, vertexData.inPositionY, vertexData.inPositionZ, 1.0f);
    vec4 inNormals = vec4(vertexData.inNormalX, vertexData.inNormalY, vertexData.inNormalZ, 1.0f);

    mat4 animationMatrix = matrices[gl_VertexIndex];
    inPosition = animationMatrix * inPosition;
    inNormals = animationMatrix * inNormals;


    vec3 inTangent = vec3(vertexData.inTangentX, vertexData.inTangentY, vertexData.inTangentZ);
    vec3 inBitangent = vec3(vertexData.inBiTangentX, vertexData.inBiTangentY, vertexData.inBiTangentZ);

    gl_Position = worldUniforms.viewproj * instanceData.matrix * inPosition;

    outPosition = (instanceData.matrix * inPosition).xyz;
    outNormals = normalize(inNormals).xyz;
    outUV = vec2(vertexData.inUvX, vertexData.inUvY);
    outCameraTBN = worldUniforms.camera.xyz;
    outLightTBN = worldUniforms.lightLocationAndStrength.xyz;
    outMeshData = meshData;

    //normal map
    if (meshData.normalTexture != -1) {
        mat3 model3 = mat3(instanceData.matrix);

        vec3 T = normalize(model3 * inTangent);
        vec3 N = normalize(model3 * outNormals);
        vec3 B = normalize(model3 * inBitangent);

        T = normalize(T - dot(T, N) * N);
        if (dot(cross(T, N), B) < 0.0f) T *= -1.0f;
        mat3 TBN = mat3(T, cross(T, N), N);

        outPosition = TBN * outPosition;
        outCameraTBN = TBN * outCameraTBN;
        outLightTBN = TBN * outLightTBN;
    }
}

