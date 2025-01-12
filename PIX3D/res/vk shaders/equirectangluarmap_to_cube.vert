#version 460 core

#extension GL_KHR_vulkan_glsl : enable

// in
layout (location = 0) in vec3 in_Position;

// out
layout (location = 0) out vec3 out_WorldCoordinates;

layout(push_constant) uniform PushConstants {
    mat4 view_projection;
} push;

void main()
{
    out_WorldCoordinates = in_Position;
    gl_Position = push.view_projection * vec4(in_Position, 1.0);
}
