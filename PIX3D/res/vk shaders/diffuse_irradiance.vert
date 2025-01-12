#version 460 core

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 in_Position;

layout (location = 0) out vec3 out_ModelCoordinates;

layout(push_constant) uniform PushConstants {
    mat4 view_projection;
} push;

void main()
{
	gl_Position = push.view_projection * vec4(in_Position, 1.0f);
	out_ModelCoordinates = in_Position;
}
