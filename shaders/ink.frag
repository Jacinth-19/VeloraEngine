#version 450

// Sumi-e ink wash: posterized two/three tone shading with dithered edges
// and a heavy dark rim, like brush ink on rice paper.

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

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-frame.lightDir.xyz);
    vec3 V = normalize(frame.cameraPos.xyz - vWorldPos);
    float lit = dot(N, L);

    // Dithered tone thresholds for a hand-brushed edge.
    float n = hash12(gl_FragCoord.xy);
    vec3 base = pc.colorA.rgb;
    vec3 inkDark = mix(pc.outlineColor.rgb, base * 0.45, 0.4);
    vec3 paper = vec3(0.955, 0.935, 0.865);
    vec3 mid = mix(base, paper, 0.15) * 0.82;
    vec3 lightCol = mix(paper, base, 0.30);

    vec3 col;
    if (lit > 0.52 + n * 0.10)      col = lightCol;
    else if (lit > 0.16 + n * 0.10) col = mid;
    else                            col = inkDark;

    // Dark ink gathers at silhouettes.
    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.5);
    col = mix(col, inkDark, smoothstep(0.5, 0.72, rim) * 0.9);

    outColor = vec4(col, 1.0);
}
