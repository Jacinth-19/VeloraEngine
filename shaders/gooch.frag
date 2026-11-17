#version 450

// Gooch shading: warm key / cool fill technical-illustration look.

layout(set = 0, binding = 0) uniform Frame {
    mat4 view;
    mat4 proj;
    vec4 lightDir;
    vec4 cameraPos;
    vec4 screen;
} frame;

layout(push_constant) uniform Push {
    mat4 model;
    vec4 colorA;       // baseColor.rgb, bands
    vec4 colorB;       // specColor.rgb, shininess
    vec4 params;
    vec4 outlineColor;
} pc;

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-frame.lightDir.xyz);
    vec3 V = normalize(frame.cameraPos.xyz - vWorldPos);
    vec3 base = pc.colorA.rgb;

    float t = 0.5 * (1.0 + dot(N, L));
    // Keep the material hue dominant; the warm/cool wash rides on top.
    vec3 cool = vec3(0.20, 0.22, 0.42) * 0.55 + 0.60 * base;
    vec3 warm = vec3(0.85, 0.72, 0.40) * 0.40 + 0.70 * base;
    vec3 col = mix(cool, warm, t);

    // Crisp white highlight.
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), pc.colorB.w);
    col = mix(col, vec3(1.0), smoothstep(0.5, 0.56, spec) * 0.85);

    // Cool rim to separate from the paper.
    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    col += vec3(0.35, 0.40, 0.55) * smoothstep(0.6, 0.75, rim) * 0.4;

    outColor = vec4(col, 1.0);
}
