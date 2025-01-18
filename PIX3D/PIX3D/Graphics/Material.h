#pragma once
#include <Platfrom/Vulkan/VulkanDescriptorSet.h>
#include <glm/glm.hpp>
#include <vector>

namespace PIX3D
{
    class SpriteMaterial
    {
    public:
        struct _ShadereData
        {
            alignas(16) glm::vec4 color;          // 16 bytes, aligned to 16
            alignas(4) float smoothness;          // 4 bytes
            alignas(4) float corner_radius;       // 4 bytes
            alignas(4) float use_texture;         // 4 bytes
            alignas(4) float tiling_factor;       // 4 bytes
            alignas(4) int flip;                  // 4 bytes
            alignas(8) glm::vec2 uv_offset;       // 8 bytes, aligned to 8
            alignas(8) glm::vec2 uv_scale;        // 8 bytes, aligned to 8
            alignas(4) int apply_uv_scale_and_offset; // 4 bytes
        };
    public:
        void Create(VK::VulkanTexture* texture = nullptr);
        void Destroy();

        void ChangeTexture(VK::VulkanTexture* new_texture);

        VK::VulkanTexture* GetTexture() { return m_Texture; }
        VkDescriptorSet GetVKDescriptorSet() { return m_DescriptorSet.GetVkDescriptorSet(); }
        void UpdateBuffer();
        void DestroyAccumelatedTextures();
    public:
        _ShadereData* m_Data;
        VK::VulkanTexture* m_Texture = nullptr;
        VK::VulkanDescriptorSet m_DescriptorSet;
        VK::VulkanShaderStorageBuffer m_DataBuffer;

        std::vector<VK::VulkanTexture*> m_AccumelatedTextures;
    };
}
