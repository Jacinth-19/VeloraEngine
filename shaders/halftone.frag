#version 450

// Halftone: the shaded color is screened through a rotated dot grid, like
// newspaper print. Darker tones get bigger dots.

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

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-frame.lightDir.xyz);
    float lit = clamp(dot(N, L), 0.0, 1.0);

    vec3 base = pc.colorA.rgb;
    vec3 shaded = base * (0.22 + 0.88 * lit);
    float lum = clamp(dot(shaded, vec3(0.299, 0.587, 0.114)), 0.0, 1.0);

    // Rotated screen-space dot grid.
    const float angle = 0.261799; // 15 degrees
    mat2 rot = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
    vec2 grid = (rot * gl_FragCoord.xy) / 9.0;
    vec2 cell = fract(grid) - 0.5;
    float d = length(cell);
    float radius = (1.0 - lum) * 0.62;
    float aa = fwidth(d) + 1e-4;
    float dotMask = 1.0 - smoothstep(radius - aa, radius + aa, d);

    vec3 paperArea = shaded * 1.18 + vec3(0.05);
    vec3 dotArea = mix(pc.outlineColor.rgb, shaded * 0.42, 0.55);
    vec3 col = mix(paperArea, dotArea, dotMask);

    outColor = vec4(col, 1.0);
}
