#include "FrostedGlassPostProcessPass.h"
#include <Engine/Engine.hpp>
#include <Platfrom/Vulkan/VulkanHelper.h>

namespace PIX3D::VK
{
    void FrostedGlassPostProcessPass::Init(uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;
        auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

        m_Shader.LoadFromFile("../PIX3D/res/vk shaders/Frosted Glass Example/frostedglass_post.vert", "../PIX3D/res/vk shaders/Frosted Glass Example/frostedglass_post.frag");

        m_ColorAttachmentTexture = new VulkanTexture();
        m_ColorAttachmentTexture->CreateColorAttachment(width, height, VK_FORMAT_R16G16B16A16_SFLOAT);

        // transition from undefined to general
        VulkanTextureHelper::TransitionImageLayout(
            m_ColorAttachmentTexture->GetImage(),
            m_ColorAttachmentTexture->GetFormat(),
            0, 1,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL
        );

        m_DescriptorSetLayout
            .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // Scene Color
            .AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // Glass Mask
            .Build();

        m_Renderpass
            .Init(Context->m_Device)
            .AddColorAttachment(
                m_ColorAttachmentTexture->GetFormat(),
                VK_SAMPLE_COUNT_1_BIT,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_GENERAL)
            .AddSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
            .AddDependency(
                VK_SUBPASS_EXTERNAL,
                0,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_MEMORY_READ_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_DEPENDENCY_BY_REGION_BIT
            )
            .AddDependency(
                0,
                VK_SUBPASS_EXTERNAL,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_DEPENDENCY_BY_REGION_BIT
            )
            .Build();

        m_Framebuffer
            .Init(Context->m_Device, m_Renderpass.GetVKRenderpass(), width, height)
            .AddAttachment(m_ColorAttachmentTexture)
            .Build();

        VkDescriptorSetLayout layouts[] = {
            m_DescriptorSetLayout.GetVkDescriptorSetLayout()
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = layouts;

        if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
            PIX_ASSERT_MSG(false, "Failed to create pipeline layout!");

        m_QuadMesh = VulkanStaticMeshGenerator::GenerateQuad();
        auto bindingDesc = m_QuadMesh.VertexLayout.GetBindingDescription();
        auto attributeDesc = m_QuadMesh.VertexLayout.GetAttributeDescriptions();

        m_GraphicsPipeline.Init(Context->m_Device, m_Renderpass.GetVKRenderpass())
            .AddShaderStages(m_Shader.GetVertexShader(), m_Shader.GetFragmentShader())
            .AddVertexInputState(&bindingDesc, attributeDesc.data(), 1, attributeDesc.size())
            .AddViewportState(width, height)
            .AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
            .AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
            .AddDepthStencilState(false, false)
            .AddColorBlendState(true, 1)
            .SetPipelineLayout(m_PipelineLayout)
            .Build();
    }

    void FrostedGlassPostProcessPass::Render(VulkanTexture* sceneColor, VulkanTexture* glassMask)
    {
        auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

        if (!m_DescriptorSet.GetVkDescriptorSet())
        {
            m_DescriptorSet
                .Init(m_DescriptorSetLayout)
                .AddTexture(0, *sceneColor)
                .AddTexture(1, *glassMask)
                .Build();
        }
        VkClearValue clearValue = {};
        clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_Renderpass.GetVKRenderpass();
        renderPassInfo.framebuffer = m_Framebuffer.GetVKFramebuffer();
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { m_Width, m_Height };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearValue;

        vkCmdBeginRenderPass(VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VulkanSceneRenderer::s_ImageIndex],
            &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VulkanSceneRenderer::s_ImageIndex],
            VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline.GetVkPipeline());

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = (float)m_Height;
        viewport.width = (float)m_Width;
        viewport.height = -(float)m_Height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { m_Width, m_Height };

        vkCmdSetViewport(VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VulkanSceneRenderer::s_ImageIndex], 0, 1, &viewport);
        vkCmdSetScissor(VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VulkanSceneRenderer::s_ImageIndex], 0, 1, &scissor);

        auto descriptorSet = m_DescriptorSet.GetVkDescriptorSet();
        vkCmdBindDescriptorSets(VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VulkanSceneRenderer::s_ImageIndex],
            VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        VkBuffer vertexBuffers[] = { m_QuadMesh.VertexBuffer.GetBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VulkanSceneRenderer::s_ImageIndex],
            0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VulkanSceneRenderer::s_ImageIndex],
            m_QuadMesh.IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VulkanSceneRenderer::s_ImageIndex],
            6, 1, 0, 0, 0);

        vkCmdEndRenderPass(VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VulkanSceneRenderer::s_ImageIndex]);
    }

    void FrostedGlassPostProcessPass::OnResize(uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;
        auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

        if (m_ColorAttachmentTexture) {
            m_ColorAttachmentTexture->Destroy();
            delete m_ColorAttachmentTexture;
        }

        m_ColorAttachmentTexture = new VulkanTexture();
        m_ColorAttachmentTexture->CreateColorAttachment(width, height, VK_FORMAT_R16G16B16A16_SFLOAT);

        m_Framebuffer.Destroy();
        m_Framebuffer
            .Init(Context->m_Device, m_Renderpass.GetVKRenderpass(), width, height)
            .AddAttachment(m_ColorAttachmentTexture)
            .Build();
    }

    void FrostedGlassPostProcessPass::Destroy()
    {
        auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

        vkDeviceWaitIdle(Context->m_Device);

        if (m_ColorAttachmentTexture) {
            m_ColorAttachmentTexture->Destroy();
            delete m_ColorAttachmentTexture;
            m_ColorAttachmentTexture = nullptr;
        }

        m_Renderpass.Destroy();
        m_Framebuffer.Destroy();
        m_Shader.Destroy();
        m_GraphicsPipeline.Destroy();
        m_DescriptorSetLayout.Destroy();
        m_DescriptorSet.Destroy();
        m_QuadMesh.VertexBuffer.Destroy();
        m_QuadMesh.IndexBuffer.Destroy();

        if (m_PipelineLayout) {
            vkDestroyPipelineLayout(Context->m_Device, m_PipelineLayout, nullptr);
            m_PipelineLayout = nullptr;
        }
    }
}
