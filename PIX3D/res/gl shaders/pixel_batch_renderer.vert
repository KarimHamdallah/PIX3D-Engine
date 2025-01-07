#version 460 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_coords;
layout (location = 2) in vec4 in_color;
layout (location = 3) in float in_circle_quad;

// out
layout (location = 0) out vec2 out_coords;
layout (location = 1) out vec4 out_color;
layout (location = 2) out float out_circle_quad;

void main()
{
    out_color = in_color;
    out_coords = in_coords;
    out_circle_quad = in_circle_quad;
    gl_Position = vec4(in_position, 1.0);
}
