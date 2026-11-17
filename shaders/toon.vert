#version 450

// Velora shared vertex stage for all stylized fill pipelines.

layout(set = 0, binding = 0) uniform Frame {
    mat4 view;
    mat4 proj;
    vec4 lightDir;   // xyz: direction the light travels
    vec4 cameraPos;  // xyz: eye
    vec4 screen;     // xy: render-target size in px
} frame;

layout(push_constant) uniform Push {
    mat4 model;
    vec4 colorA;       // baseColor.rgb, bands
    vec4 colorB;       // specColor.rgb, shininess
    vec4 params;       // rimStrength, rimPower, outlineWidth
    vec4 outlineColor;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;

void main() {
    vec4 world = pc.model * vec4(inPosition, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(pc.model) * inNormal; // nodes use uniform scale only
    gl_Position = frame.proj * frame.view * world;
}
