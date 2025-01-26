#pragma once
#include <Platfrom/Vulkan/VulkanRenderpass.h>
#include <Platfrom/Vulkan/VulkanFramebuffer.h>
#include <Platfrom/Vulkan/VulkanShader.h>
#include <Platfrom/Vulkan/VulkanGraphicsPipeline.h>
#include <Platfrom/Vulkan/VulkanTexture.h>
#include <Graphics/VulkanStaticMesh.h>
#include <Scene/Scene.h>

namespace PIX3D
{
    class FrostedGlassMaskRenderpass
    {
    public:
        void Init(uint32_t width, uint32_t height);
        void Render(VulkanStaticMesh& mesh, const glm::mat4& transform);
        void OnResize(uint32_t width, uint32_t height);
        void Destroy();

        VK::VulkanTexture* GetOutputTexture() { return m_ColorAttachmentTexture; }

    private:
        struct PushConstant {
            glm::mat4 model;
        };

        VK::VulkanShader m_Shader;
        VK::VulkanRenderPass m_Renderpass;
        VK::VulkanFramebuffer m_Framebuffer;
        VkPipelineLayout m_PipelineLayout = nullptr;
        VK::VulkanGraphicsPipeline m_GraphicsPipeline;
        VK::VulkanTexture* m_ColorAttachmentTexture = nullptr;
    };
}
