#version 460 core

#extension GL_ARB_bindless_texture : require

// in
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec3 in_Tangent;
layout (location = 3) in vec3 in_BiTangent;
layout (location = 4) in vec2 in_TexCoords;

// out
layout (location = 0) out vec2 out_TexCoords;
layout (location = 1) out vec3 out_WorldCoordinates;
layout (location = 2) out vec3 out_Tangent;
layout (location = 3) out vec3 out_BiTangent;
layout (location = 4) out vec3 out_Normal;



// uniform buffer
layout (binding = 0, std140) uniform u_camera
{
     mat4 proj;
     mat4 view;
     mat4 skybox_view;
}cam;

// uniforms
layout (location = 0) uniform mat4 model;

void main()
{
    out_WorldCoordinates = (model * vec4(in_Position, 1.0f)).xyz;
	
	gl_Position = cam.proj * cam.view * model * vec4(in_Position, 1.0f);
	
	out_TexCoords = in_TexCoords;

	mat3 normalMatrix = transpose(inverse(mat3(model)));

	out_Tangent = normalize(normalMatrix * in_Tangent);
	out_BiTangent = normalize(normalMatrix * in_BiTangent);
	out_Normal = normalize(normalMatrix * in_Normal);
}
