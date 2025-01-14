#pragma once
#include <Platfrom/Vulkan/VulkanDescriptorSet.h>
#include <glm/glm.hpp>

namespace PIX3D
{
    class SpriteMaterial
    {
    public:
        struct _ShadereData
        {
            glm::vec4 color;
            float smoothness;
            float corner_radius;
            float use_texture;
            float tiling_factor;
            int flip;
            glm::vec2 uv_offset;
            glm::vec2 uv_scale;
            int apply_uv_scale_and_offset;
        };
    public:
        void Create(VK::VulkanTexture* texture = nullptr);
        void Destroy();

        VK::VulkanTexture* GetTexture() { return m_Texture; }
        VkDescriptorSet GetVKDescriptorSet() { return m_DescriptorSet.GetVkDescriptorSet(); }

    private:
        void UpdateBuffer();
    public:
        _ShadereData* m_Data;
        VK::VulkanTexture* m_Texture = nullptr;
        VK::VulkanDescriptorSet m_DescriptorSet;
        VK::VulkanShaderStorageBuffer m_DataBuffer;
    };
}
