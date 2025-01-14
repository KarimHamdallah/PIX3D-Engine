#version 460

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_coords;
layout(location = 2) in vec2 in_uvs;

// camera uniform buffer
layout(set = 0, binding = 0) uniform CameraUniformBuffer
{
    mat4 proj;
    mat4 view;
    mat4 skybox_view;
}cam;

layout(push_constant) uniform PushConstants
{
   mat4 model;
}push;

layout(location = 0) out vec2 out_coords;
layout(location = 1) out vec2 out_uvs;

void main() 
{
    out_coords = in_coords;
    out_uvs = in_uvs;
    gl_Position = cam.proj * cam.view * push.model * vec4(in_position, 1.0);
}
