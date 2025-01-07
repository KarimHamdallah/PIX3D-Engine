#version 460 core

layout (location = 0) in vec3 in_position;

layout (location = 0) uniform mat4 model;

layout (binding = 0, std140) uniform u_camera
{
     mat4 proj;
}cam;

void main()
{
    gl_Position = cam.proj * model * vec4(in_position, 1.0);
}
