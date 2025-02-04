#version 460

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BloomBrightnessColor;

layout(location = 0) in vec2 in_coords;
layout(location = 1) in vec2 in_uvs;

layout(set = 1, binding = 1) uniform sampler2D u_texture;

void main()
{
    vec4 color = texture(u_texture, vec2(in_uvs.x, 1.0 - in_uvs.y));
    if(color.a < 0.1) discard;
    FragColor = color;
    BloomBrightnessColor = vec4(0.0, 0.0, 0.0, 1.0);
}
