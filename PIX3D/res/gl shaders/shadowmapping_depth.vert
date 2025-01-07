#version 460 core

// in
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec3 in_Tangent;
layout (location = 3) in vec3 in_BiTangent;
layout (location = 4) in vec2 in_TexCoords;

layout (location = 0) uniform mat4 model;

void main()
{
    gl_Position = model * vec4(in_Position, 1.0);
}
