#version 460

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BloomBrightnessColor;

layout(location = 0) in vec2 in_coords;
layout(location = 1) in vec2 in_uvs;

layout(std430, set = 1, binding = 0) readonly buffer SpriteData
{
    vec4 color;
    float smoothness;
    float corner_radius;
    float use_texture;
    float tiling_factor;
    bool flip;
    vec2 uv_offset;
    vec2 uv_scale;
    bool apply_uv_scale_and_offset;
} sprite;

layout(set = 1, binding = 1) uniform sampler2D u_texture;

void main()
{
    vec4 color = sprite.color;
    
    if(sprite.use_texture > 0.5)
    {
        vec2 TexCoords = in_uvs;
        if(sprite.flip)
        {
            TexCoords = vec2(in_uvs.s, 1.0 - in_uvs.t);
        }
        if(sprite.apply_uv_scale_and_offset)
        {
            TexCoords = (TexCoords * sprite.uv_scale) + sprite.uv_offset;
        }
        color *= texture(u_texture, TexCoords * sprite.tiling_factor);
        
        if(color.a < 0.1) discard;
        
        FragColor = vec4(color);
    }
    else
    {
        vec2 localPos = abs(in_coords - vec2(0.0)) - (1.0 - sprite.corner_radius);
        float dist = length(max(localPos, 0.0)) - sprite.corner_radius;
        float alpha = smoothstep(0.0, sprite.smoothness, -dist);
        FragColor = vec4(color.rgb, color.a * alpha);
    }

    BloomBrightnessColor = vec4(0.0, 0.0, 0.0, 1.0);
}
