#version 450
/*
 * axiom3d.vert — AXIOM Vulkan Expressway vertex shader
 *
 * Per-vertex input layout (matches vulkan_expressway.cpp VkVertexInputAttributeDescription):
 *   location 0 : vec3  position  (aPos)
 *   location 1 : vec3  normal    (aNormal)
 *   location 2 : vec3  colour    (aColor)   — height-based gradient from Python
 *
 * Push constants (96 bytes, shared with fragment shader):
 *   mat4  mvp       — column-major MVP transform
 *   vec4  lightDir  — xyz world-space light direction  + w = ambient strength
 *   vec4  viewPos   — xyz camera world-space position  + w = padding
 */

#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

layout(push_constant) uniform PC {
    mat4 mvp;
    vec4 lightDir;
    vec4 viewPos;
} pc;

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec3 vWorldPos;

void main() {
    gl_Position = pc.mvp * vec4(aPos, 1.0);
    vWorldPos   = aPos;       // identity model → world == local
    vNormal     = aNormal;    // identity model → no TBN transform needed
    vColor      = aColor;
}
