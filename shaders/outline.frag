#version 450

// Flat ink color for the inverted-hull outline pass.

layout(set = 0, binding = 0) uniform Frame {
    mat4 view;
    mat4 proj;
    vec4 lightDir;
    vec4 cameraPos;
    vec4 screen;
} frame;

layout(push_constant) uniform Push {
    mat4 model;
    vec4 colorA;
    vec4 colorB;
    vec4 params;
    vec4 outlineColor;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(pc.outlineColor.rgb, 1.0);
}
