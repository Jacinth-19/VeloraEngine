#version 450

// Cel shading: band-quantized diffuse, stepped specular highlight, rim light.

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
    vec4 params;       // rimStrength, rimPower, outlineWidth
    vec4 outlineColor;
} pc;

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-frame.lightDir.xyz);
    vec3 V = normalize(frame.cameraPos.xyz - vWorldPos);

    // Band-quantized diffuse with a small AA transition at band edges.
    float bands = max(pc.colorA.w, 1.0);
    float scaled = clamp(dot(N, L), 0.0, 1.0) * bands;
    float band = floor(scaled);
    float edge = fract(scaled);
    float aa = fwidth(scaled) * 1.5 + 1e-4;
    float t = smoothstep(0.5 - aa, 0.5 + aa, edge);
    float shade = (band + t) / bands;

    vec3 base = pc.colorA.rgb;
    vec3 col = base * (0.24 + 0.76 * shade);

    // Stepped specular highlight.
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), pc.colorB.w);
    col += pc.colorB.rgb * smoothstep(0.45, 0.55, spec) * 0.55;

    // Rim light.
    float rim = pow(1.0 - max(dot(N, V), 0.0), pc.params.y);
    col += vec3(1.0, 0.98, 0.9) * smoothstep(0.55, 0.65, rim) * pc.params.x;

    outColor = vec4(col, 1.0);
}
