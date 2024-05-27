#version 460

layout(binding = 0) uniform UniformBufferObject {
    mat4 viewproj;
    vec4 camera;
    vec4 lightColorAndGlossiness;
    vec4 lightLocationAndStrength;
} worldUniforms;

layout(push_constant) uniform PushConstants {
    vec4 color;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormals;

layout(location = 0) out vec4 outColor;

void main() {

    vec3 ambient_light = worldUniforms.lightColorAndGlossiness.rgb * worldUniforms.lightLocationAndStrength.a;
    vec3 ambient_color = ambient_light * pushConstants.color.rgb;

    vec3 lightDirection = normalize(worldUniforms.lightLocationAndStrength.xyz - inPosition);
    float diffuse = max(dot(inNormals, lightDirection), 0.0);
    vec3 diffuse_color = diffuse * pushConstants.color.rgb * worldUniforms.lightLocationAndStrength.a;

    vec3 eyeDirection = normalize(worldUniforms.camera.xyz - inPosition);
    vec3 reflection = reflect(-lightDirection, inNormals);
    float specular = pow(max(dot(eyeDirection, reflection), 0.0), worldUniforms.lightColorAndGlossiness.a);
    vec3 specular_color = specular * ambient_light;

    outColor = vec4(diffuse_color.rgb + ambient_color.rgb + specular_color.rgb, pushConstants.color.a);

}
