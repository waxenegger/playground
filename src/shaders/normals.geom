#version 460

layout(triangles) in;
layout (line_strip, max_vertices = 2) out;

layout(location = 0) in vec3 inColor[];
layout (location = 1) in vec3 inNormals[];


layout(location = 0) out vec3 outColor;

void main() {

    float normalLength = 0.75;
    for (int i = 0; i < gl_in.length(); i++) {
        outColor = inColor[i];

        gl_Position = gl_in[i].gl_Position;

        EmitVertex();

        gl_Position = gl_in[i].gl_Position + vec4(normalize(inNormals[i]) * normalLength, 0);

        EmitVertex();

        EndPrimitive();
    }
}

