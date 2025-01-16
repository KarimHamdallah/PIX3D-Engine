#version 460 core

#extension GL_KHR_vulkan_glsl : enable

// in
layout (location = 0) in vec3 in_Position;

// out
layout (location = 0) out vec3 out_TextureCoords;

// camera uniform buffer
layout(set = 0, binding = 0) uniform CameraUniformBuffer
{
    mat4 proj;
    mat4 view;
    mat4 skybox_view;
}cam;

layout(push_constant) uniform PushConstants {
    mat4 model;
   vec3 CameraPosition;
   float MeshIndex;
   float BloomThreshold;
}push;

void main()
{
    vec4 position = cam.proj * cam.skybox_view * push.model * vec4(in_Position, 1.0f);

	// we set z = w so that after perspective divide z will be 1.0, which is max depth value
	// this keeps the skybox behind everything
	// got z-fighting for (w / w), hence the subtractio to make the depth a little less than max
	gl_Position = vec4(position.xy, position.w, position.w);

	out_TextureCoords = in_Position; // treat position as a vector for cubemap texture coords
}
