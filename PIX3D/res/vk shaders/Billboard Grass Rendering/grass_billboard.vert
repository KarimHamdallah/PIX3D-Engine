#version 460

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_coords;
layout(location = 2) in vec2 in_uvs;

// camera uniform buffer
layout(set = 0, binding = 0) uniform CameraUniformBuffer
{
    mat4 proj;
    mat4 view;
    mat4 skybox_view;
}cam;

layout(std430, set = 1, binding = 0) readonly buffer TransformationsBuffer
{
	 mat4 data[];
}transformations;

layout(location = 0) out vec2 out_coords;
layout(location = 1) out vec2 out_uvs;

// Classic 2D noise function (put this before main)
vec2 hash2(vec2 p)
{
    p = vec2(dot(p,vec2(127.1,311.7)),
             dot(p,vec2(269.5,183.3)));
    return -1.0 + 2.0 * fract(sin(p)*43758.5453123);
}

float noise2d(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    
    vec2 u = f*f*(3.0-2.0*f);

    return mix(mix(dot(hash2(i + vec2(0.0,0.0)), f - vec2(0.0,0.0)),
                   dot(hash2(i + vec2(1.0,0.0)), f - vec2(1.0,0.0)), u.x),
               mix(dot(hash2(i + vec2(0.0,1.0)), f - vec2(0.0,1.0)),
                   dot(hash2(i + vec2(1.0,1.0)), f - vec2(1.0,1.0)), u.x), u.y);
}

layout(push_constant) uniform PushConstants
{
   float time;
   float wind_strength;
   vec2 wind_movement;
}push;

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

void main() 
{
    vec4 worldPos = transformations.data[gl_InstanceIndex] * vec4(in_position, 1.0);
    vec2 windUV = worldPos.xy;
    vec2 timeOffset = push.time * push.wind_movement;
    float windNoise = noise2d(windUV + timeOffset);
    windNoise = windNoise - 0.5;
    windNoise = windNoise * push.wind_strength;

    // Use in_uvs.y as our lerp factor (0 at bottom, 1 at top)
    float heightFactor = in_uvs.y;  // Bottom=0, Top=1
    float windEffect = lerp(0.0, windNoise, heightFactor);
    
    vec4 position = worldPos;
    position.x = worldPos.x + windEffect;  // Now using lerped wind effect
    
    out_coords = in_coords;
    out_uvs = in_uvs;
    gl_Position = cam.proj * cam.view * transformations.data[gl_InstanceIndex] * vec4(position.xyz, 1.0);
}
