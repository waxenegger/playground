#version 460
#extension GL_EXT_nonuniform_qualifier: enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 viewproj;
    vec4 camera;
    vec4 lightColorAndGlossiness;
    vec4 lightLocationAndStrength;
} worldUniforms;

#ifdef GL_EXT_nonuniform_qualifier
layout(binding = 2) uniform sampler2D samplers[];
#else
layout(binding = 2) uniform sampler2D samplers[5000];
#endif

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormals;
layout(location = 2) in vec2 inUV;
layout(location = 3) in flat uint inTextureId;

layout(location = 0) out vec4 outColor;

void main() {
    float ambientFactor = 0.01;

    vec3 ambient_light = worldUniforms.lightColorAndGlossiness.rgb * worldUniforms.lightLocationAndStrength.a;
    vec3 ambient_color = ambient_light * ambientFactor;

    vec3 lightDirection = normalize(worldUniforms.lightLocationAndStrength.xyz - inPosition);
    float diffuse = max(dot(inNormals, lightDirection), 0.1);
    vec3 diffuse_color = diffuse * worldUniforms.lightColorAndGlossiness.rgb * worldUniforms.lightLocationAndStrength.a;

    vec3 eyeDirection = normalize(worldUniforms.camera.xyz - inPosition);
    vec3 reflection = reflect(-lightDirection, inNormals);
    float specular = pow(max(dot(eyeDirection, reflection), 0.1), worldUniforms.lightColorAndGlossiness.a);
    vec3 specular_color = specular * ambient_color;

    outColor = vec4(texture(samplers[inTextureId], inUV).rgb * (ambient_color + diffuse_color + specular_color), 1.0f);

}
