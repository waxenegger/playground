#version 440

layout(binding = 0) uniform UniformBufferObject {
    mat4 viewproj;
    vec4 camera;
    vec4 lightColorAndGlossiness;
    vec4 lightLocationAndStrength;
} worldUniforms;

layout(location = 0) in vec3 inPosition;

layout(binding = 2) uniform samplerCube skybox;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 inNormals = normalize(inPosition);
    vec3 inColor = vec3(1.0f);

    //vec3 ambient_light = worldUniforms.lightColorAndGlossiness.rgb * worldUniforms.lightLocationAndStrength.a;
    vec3 ambient_color = worldUniforms.lightLocationAndStrength.a * inColor;

    vec3 lightDirection = normalize(inPosition - worldUniforms.lightLocationAndStrength.xyz);
    float diffuse = max(dot(inNormals, lightDirection), 0.0);
    vec3 diffuse_color = diffuse * inColor * worldUniforms.lightLocationAndStrength.a;

    vec3 eyeDirection = normalize(worldUniforms.camera.xyz - inPosition);
    vec3 reflection = reflect(-lightDirection, inNormals);
    float specular = pow(max(dot(eyeDirection, reflection), 0.0), worldUniforms.lightColorAndGlossiness.a);
    vec3 specular_color = specular * ambient_color;

    vec4 lightContribution = vec4(diffuse_color.rgb + ambient_color.rgb + specular_color.rgb, 1.0);

    outColor = texture(skybox, inPosition) * lightContribution;
}

