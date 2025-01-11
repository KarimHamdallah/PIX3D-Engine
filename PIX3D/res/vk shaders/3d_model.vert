#version 460

#extension GL_KHR_vulkan_glsl : enable

// in
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec3 in_Tangent;
layout (location = 3) in vec3 in_BiTangent;
layout (location = 4) in vec2 in_TexCoords;

// out
layout (location = 0) out vec2 out_TexCoords;

// camera uniform buffer
layout(set = 0, binding = 0) uniform CameraUniformBuffer
{
    mat4 view;
    mat4 proj;
}cam;

void main()
{
    gl_Position = cam.proj * cam.view * vec4(in_Position, 1.0);
    out_TexCoords = in_TexCoords;
}
