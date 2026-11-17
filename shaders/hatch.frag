#version 450

// Screen-space cross-hatching: the darker the surface, the more pen strokes
// pile up. Stroke direction rotates per tone level, like a pencil drawing.

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

// Periodic dark line along `dir`, spacing/width in pixels. Returns ink amount.
float hatchLine(vec2 p, vec2 dir, float spacing, float width) {
    float s = abs(sin(dot(p, dir) * 3.14159265 / spacing));
    float aa = fwidth(s) + 1e-4;
    return 1.0 - smoothstep(width - aa, width + aa, s);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-frame.lightDir.xyz);
    float lit = clamp(dot(N, L) * 0.5 + 0.5, 0.0, 1.0);

    vec2 p = gl_FragCoord.xy;
    float ink = 0.0;
    if (lit < 0.85) ink = max(ink, hatchLine(p, vec2( 0.7071, 0.7071), 6.5, 0.28));
    if (lit < 0.60) ink = max(ink, hatchLine(p, vec2(-0.7071, 0.7071), 6.0, 0.28));
    if (lit < 0.38) ink = max(ink, hatchLine(p, vec2( 1.0,    0.0   ), 5.5, 0.24));
    if (lit < 0.20) ink = max(ink, hatchLine(p, vec2( 0.0,    1.0   ), 5.5, 0.24));

    vec3 paper = vec3(0.965, 0.945, 0.885);
    vec3 tintedPaper = paper * mix(vec3(1.0), pc.colorA.rgb, 0.45);
    vec3 shaded = tintedPaper * (0.30 + 0.70 * lit);
    vec3 col = mix(shaded, pc.outlineColor.rgb, ink * 0.85);

    outColor = vec4(col, 1.0);
}
