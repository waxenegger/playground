#version 460
#extension GL_EXT_nonuniform_qualifier: enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 viewproj;
    vec4 camera;
    vec4 lightColorAndGlossiness;
    vec4 lightLocationAndStrength;
} worldUniforms;

struct MeshData {
    vec4 color;
    vec4 specularColor;
    int ambientTexture;
    int diffuseTexture;
    int specularTexture;
    int normalTexture;
};

#ifdef GL_EXT_nonuniform_qualifier
layout(binding = 5) uniform sampler2D samplers[];
#else
layout(binding = 5) uniform sampler2D samplers[5000];
#endif

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormals;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inCameraTBN;
layout(location = 4) in vec3 inLightTBN;
layout(location = 5) flat in MeshData inMeshData;

layout(location = 0) out vec4 outColor;


void main() {
    float ambientFactor = 0.01;

    vec3 ambient_light = worldUniforms.lightColorAndGlossiness.rgb * worldUniforms.lightLocationAndStrength.a;
    vec3 ambient_color = ambient_light * ambientFactor;

    // normal map
    vec3 normals = inNormals;
    if (inMeshData.normalTexture != -1) {
        normals = texture(samplers[inMeshData.normalTexture], inUV).rgb;
        normals = normalize(normals);
    }

    vec3 lightDirection = normalize(inLightTBN - inPosition);
    float diffuse = max(dot(normals, lightDirection), 0.1);
    vec3 diffuse_color = diffuse * inMeshData.color.rgb * worldUniforms.lightLocationAndStrength.a;

    vec3 eyeDirection = normalize(inCameraTBN - inPosition);
    vec3 reflection = reflect(-lightDirection, normals);

    float glossy = inMeshData.specularColor.a == 0 ? worldUniforms.lightColorAndGlossiness.a : inMeshData.specularColor.a;
    float specular = pow(max(dot(eyeDirection, reflection), 0), glossy);
    vec3 specular_color = specular * inMeshData.specularColor.rgb * worldUniforms.lightLocationAndStrength.a * ambientFactor;

    // ambience
    if (inMeshData.ambientTexture != -1) {
        ambient_color = texture(samplers[inMeshData.ambientTexture], inUV).rgb * ambientFactor;
    } else if (inMeshData.diffuseTexture != -1) {
        ambient_color = texture(samplers[inMeshData.diffuseTexture], inUV).rgb * ambientFactor;
    }

    // diffuse
    if (inMeshData.diffuseTexture != -1) {
        diffuse_color = texture(samplers[inMeshData.diffuseTexture], inUV).rgb * diffuse * worldUniforms.lightLocationAndStrength.a;
    }

    // sepcular
    if (inMeshData.specularTexture != -1) {
        specular_color = texture(samplers[inMeshData.specularTexture], inUV).rgb * specular * worldUniforms.lightLocationAndStrength.a;
    }

    outColor = vec4(ambient_color + diffuse_color + specular_color, inMeshData.color.a);
}

