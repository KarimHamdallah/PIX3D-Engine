#version 460

#extension GL_KHR_vulkan_glsl : enable

// in
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec3 in_Tangent;
layout (location = 3) in vec3 in_BiTangent;
layout (location = 4) in vec2 in_TexCoords;

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
   vec3 CameraPosition;
   float MeshIndex;
   float BloomThreshold;
   float PointLightCount;
}push;

void main()
{	
	gl_Position = cam.proj * cam.view * push.model * vec4(in_Position, 1.0f);
}
