#version 460

#extension GL_KHR_vulkan_glsl : enable

// in
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec2 in_TexCoords;

// out
layout (location = 0) out vec2 out_TexCoords;

// camera uniform buffer
layout(binding = 0) uniform CameraUniformBuffer
{
    mat4 view;
    mat4 proj;
}cam;

void main()
{
    gl_Position = cam.proj * cam.view * vec4(in_Position, 1.0);
    out_TexCoords = in_TexCoords;
}
