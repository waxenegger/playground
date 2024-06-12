#version 460
#extension GL_EXT_nonuniform_qualifier: enable


layout(binding = 0) uniform UniformBufferObject {
    mat4 viewproj;
    vec4 camera;
    vec4 lightColorAndGlossiness;
    vec4 lightLocationAndStrength;
} worldUniforms;

layout(push_constant) uniform PushConstants {
    mat4 matrix;
    vec4 color;
    vec4 specularColor;
    int ambientTexture;
    int diffuseTexture;
    int specularTexture;
    int normalTexture;
} pushConstants;

#ifdef GL_EXT_nonuniform_qualifier
layout(binding = 2) uniform sampler2D samplers[];
#else
layout(binding = 2) uniform sampler2D samplers[5000];
#endif

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormals;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main() {
    float ambientFactor = 0.01;

    vec3 ambient_light = worldUniforms.lightColorAndGlossiness.rgb * worldUniforms.lightLocationAndStrength.a;
    vec3 ambient_color = ambient_light * ambientFactor;

    vec3 lightDirection = normalize(worldUniforms.lightLocationAndStrength.xyz - inPosition);
    float diffuse = max(dot(inNormals, lightDirection), 0.1);
    vec3 diffuse_color = diffuse * pushConstants.color.rgb * worldUniforms.lightLocationAndStrength.a;

    vec3 eyeDirection = normalize(worldUniforms.camera.xyz - inPosition);
    vec3 reflection = reflect(-lightDirection, inNormals);
    float specular = pow(max(dot(eyeDirection, reflection), 0.1), pushConstants.specularColor.a);
    vec3 specular_color = specular * ambient_color;

    // ambience
    if (pushConstants.ambientTexture != -1) {
        ambient_color = texture(samplers[pushConstants.ambientTexture], inUV).rgb * ambientFactor;
    } else if (pushConstants.diffuseTexture != -1) {
        ambient_color = texture(samplers[pushConstants.diffuseTexture], inUV).rgb * ambientFactor;
    }

    // diffuse
    if (pushConstants.diffuseTexture != -1) {
        diffuse_color = texture(samplers[pushConstants.diffuseTexture], inUV).rgb * diffuse * worldUniforms.lightLocationAndStrength.a;
    }

    // sepcular
    if (pushConstants.specularTexture != -1) {
        specular_color = texture(samplers[pushConstants.specularTexture], inUV).rgb * specular * worldUniforms.lightLocationAndStrength.a;
    }

    outColor = vec4(ambient_color + diffuse_color + specular_color, pushConstants.specularColor.a);
}
