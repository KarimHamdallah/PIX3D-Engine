#include "Material.h"
#include <Platfrom/Vulkan/VulkanSceneRenderer.h>

namespace PIX3D
{
    void SpriteMaterial::Create(VK::VulkanTexture* texture)
    {
        // Create shader storage buffer
        m_DataBuffer.Create(sizeof(_ShadereData));

        // Allocate and initialize shader data
        m_Data = new _ShadereData();
        m_Data->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        m_Data->smoothness = 0.01f;
        m_Data->corner_radius = 0.01f;
        m_Data->use_texture = 1.0f;
        m_Data->tiling_factor = 1.0f;
        m_Data->flip = 1;
        m_Data->uv_offset = { 0.0f, 0.0f };
        m_Data->uv_scale = { 1.0f, 1.0f };
        m_Data->apply_uv_scale_and_offset = 0;

        // Set up texture
        if (texture)
        {
            m_Texture = texture;
            m_Data->use_texture = 1.0f;
        }
        else
        {
            m_Texture = VK::VulkanSceneRenderer::GetDefaultWhiteTexture();
            m_Data->use_texture = 0.0f;
        }

        m_DescriptorSet
            .Init(VK::VulkanSceneRenderer::s_SpriteRenderpass.DescriptorSetLayout)
            .AddShaderStorageBuffer(0, m_DataBuffer)
            .AddTexture(1, *m_Texture)
            .Build();

        UpdateBuffer();
    }

    void SpriteMaterial::UpdateBuffer()
    {
        m_DataBuffer.UpdateData(m_Data, sizeof(_ShadereData));
    }

    void SpriteMaterial::Destroy()
    {
        if (m_Texture)
        {
            m_Texture->Destroy();
            m_Texture = nullptr;
        }

        if (m_Data)
        {
            delete m_Data;
            m_Data = nullptr;
        }

        m_DataBuffer.Destroy();
        m_DescriptorSet.Destroy();
    }
}
