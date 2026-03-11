#version 450
/*
 * axiom3d.frag — AXIOM Vulkan Expressway fragment shader
 *
 * Blinn-Phong shading with height-based colour gradient from vertex buffer.
 */

#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vWorldPos;

layout(push_constant) uniform PC {
    mat4  mvp;
    vec4  lightDir;  // xyz = direction (pointing toward light), w = ambient
    vec4  viewPos;   // xyz = camera world-space position,       w = padding
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(pc.lightDir.xyz);
    vec3 V = normalize(pc.viewPos.xyz - vWorldPos);
    vec3 H = normalize(L + V);               // Blinn-Phong half-vector

    float ambient  = pc.lightDir.w;
    float diffuse  = max(dot(N, L), 0.0);
    float specular = pow(max(dot(N, H), 0.0), 64.0) * 0.45;

    vec3 lit = (ambient + diffuse + specular) * vColor;
    outColor = vec4(clamp(lit, 0.0, 1.0), 1.0);
}
