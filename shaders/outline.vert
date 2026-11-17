#version 450

// Inverted-hull outline: back faces are pushed out along the vertex normal.
// The expansion scales with distance to camera so line width stays roughly
// constant in screen space. Drawn with front-face culling.

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
    vec4 params;       // rimStrength, rimPower, outlineWidth
    vec4 outlineColor;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

void main() {
    vec4 world = pc.model * vec4(inPosition, 1.0);
    vec3 worldNormal = normalize(mat3(pc.model) * inNormal);
    float dist = distance(frame.cameraPos.xyz, world.xyz);
    world.xyz += worldNormal * pc.params.z * dist * 0.12;
    gl_Position = frame.proj * frame.view * world;
}
