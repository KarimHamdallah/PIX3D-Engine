#pragma once
#include <Platfrom/Vulkan/VulkanRenderpass.h>
#include <Platfrom/Vulkan/VulkanFramebuffer.h>
#include <Platfrom/Vulkan/VulkanShader.h>
#include <Platfrom/Vulkan/VulkanGraphicsPipeline.h>
#include <Platfrom/Vulkan/VulkanTexture.h>
#include <Platfrom/Vulkan/VulkanDescriptorSetLayout.h>
#include <Platfrom/Vulkan/VulkanDescriptorSet.h>
#include <Platfrom/Vulkan/VulkanStaticMeshFactory.h>

namespace PIX3D::VK
{
    class FrostedGlassPostProcessPass
    {
    public:
        void Init(uint32_t width, uint32_t height);
        void Render(VulkanTexture* sceneColor, VulkanTexture* glassMask);
        void OnResize(uint32_t width, uint32_t height);
        void Destroy();

        VulkanTexture* GetOutputTexture() { return m_ColorAttachmentTexture; }

    private:
        VulkanShader m_Shader;
        VulkanRenderPass m_Renderpass;
        VulkanFramebuffer m_Framebuffer;
        VkPipelineLayout m_PipelineLayout = nullptr;
        VulkanGraphicsPipeline m_GraphicsPipeline;
        VulkanTexture* m_ColorAttachmentTexture = nullptr;

        VulkanDescriptorSetLayout m_DescriptorSetLayout;
        VulkanDescriptorSet m_DescriptorSet;

        VulkanStaticMeshData m_QuadMesh;
        uint32_t m_Width = 0, m_Height = 0;
    };
}
