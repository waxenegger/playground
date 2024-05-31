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

struct IndirectDrawCommand {
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
};

layout(binding = 2) buffer indirectDrawSSBO {
    IndirectDrawCommand draws[];
};

struct InstanceData {
    mat4 matrix;
    vec4 color;
};

layout(binding = 3) buffer instanceSSBO {
    InstanceData instances[];
};


layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormals;
layout(location = 2) out vec4 outColor;

void main() {
    VertexData vertexData = vertices[gl_VertexIndex];
    IndirectDrawCommand draw = draws[gl_DrawID];
    InstanceData instanceData = instances[draw.firstInstance];


    vec4 inPosition = vec4(vertexData.inPositionX, vertexData.inPositionY, vertexData.inPositionZ, 1.0f);
    gl_Position = worldUniforms.viewproj * instanceData.matrix * inPosition;

    outPosition = inPosition.xyz;
    outColor = instanceData.color;

    vec4 transformedNormals = instanceData.matrix * vec4(vertexData.inNormalX, vertexData.inNormalY, vertexData.inNormalZ, 1.0f);
    outNormals = normalize(transformedNormals.xyz);
}
