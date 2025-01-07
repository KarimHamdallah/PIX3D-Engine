#version 460 core

layout (location = 0) in vec3 in_Position;

layout (location = 0) out vec3 out_TextureCoords;

// uniform buffer
layout (binding = 0, std140) uniform u_camera
{
     mat4 proj;
     mat4 view;
     mat4 skybox_view;
}cam;

// uniforms
layout (location = 0) uniform mat4 model;

void main() {
    vec4 position = cam.proj * cam.skybox_view * model * vec4(in_Position, 1.0f);

	// we set z = w so that after perspective divide z will be 1.0, which is max depth value
	// this keeps the skybox behind everything
	// got z-fighting for (w / w), hence the subtractio to make the depth a little less than max
	gl_Position = vec4(position.xy, position.w, position.w);

	out_TextureCoords = in_Position; // treat position as a vector for cubemap texture coords
}
