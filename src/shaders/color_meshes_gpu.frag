#version 460

layout(binding = 0) uniform UniformBufferObject {
    mat4 viewproj;
    vec4 camera;
    vec4 lightColorAndGlossiness;
    vec4 lightLocationAndStrength;
} worldUniforms;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormals;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    float ambientFactor = 0.01;

    vec3 ambient_light = worldUniforms.lightColorAndGlossiness.rgb * worldUniforms.lightLocationAndStrength.a;
    vec3 ambient_color = ambient_light * inColor.rgb * ambientFactor;

    vec3 lightDirection = normalize(worldUniforms.lightLocationAndStrength.xyz - inPosition);
    float diffuse = max(dot(inNormals, lightDirection), 0.1);
    vec3 diffuse_color = diffuse * inColor.rgb * worldUniforms.lightLocationAndStrength.a;

    vec3 eyeDirection = normalize(worldUniforms.camera.xyz - inPosition);
    vec3 reflection = reflect(-lightDirection, inNormals);
    float specular = pow(max(dot(eyeDirection, reflection), 0.1), worldUniforms.lightColorAndGlossiness.a);
    vec3 specular_color = specular * ambient_color;

    outColor = vec4(diffuse_color.rgb + ambient_color.rgb + specular_color.rgb, inColor.a);

}
