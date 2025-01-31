#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(push_constant) uniform PushConstant {
    mat4 model;
    vec3 cameraPos;
    float selectedMeshID;
    float bloomThreshold;
    float pointLightCount;
} pushConstant;

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4 projection;
    mat4 view;
    mat4 skybox_view;
} camera;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outNormal;

void main()
{
    vec4 worldPos = pushConstant.model * vec4(inPosition, 1.0);
    gl_Position = camera.projection * camera.view * worldPos;
    
    outWorldPos = worldPos.xyz;
    outTexCoord = inTexCoord;
    outNormal = mat3(pushConstant.model) * inNormal;
}
