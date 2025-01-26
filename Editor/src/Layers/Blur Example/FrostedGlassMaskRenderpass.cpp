#include "FrostedGlassMaskRenderpass.h"
#include <Engine/Engine.hpp>
#include <Platfrom/Vulkan/VulkanHelper.h>
#include <Platfrom/Vulkan/VulkanSceneRenderer.h>

namespace PIX3D
{

    void FrostedGlassMaskRenderpass::Init(uint32_t width, uint32_t height)
    {
        auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

        m_Shader.LoadFromFile("../PIX3D/res/vk shaders/Frosted Glass Example/frostedglass_mask.vert", "../PIX3D/res/vk shaders/Frosted Glass Example/frostedglass_mask.frag");

        m_ColorAttachmentTexture = new VK::VulkanTexture();
        m_ColorAttachmentTexture->CreateColorAttachment(width, height, VK_FORMAT_R16G16B16A16_SFLOAT);

        m_Renderpass
            .Init(Context->m_Device)
            .AddColorAttachment(
                m_ColorAttachmentTexture->GetFormat(),
                VK_SAMPLE_COUNT_1_BIT,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            .AddDepthAttachment(
                Context->m_SupportedDepthFormat,
                VK_SAMPLE_COUNT_1_BIT,
                VK_ATTACHMENT_LOAD_OP_LOAD,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            .AddSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
            .AddDependency(
                VK_SUBPASS_EXTERNAL,
                0,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                VK_ACCESS_MEMORY_READ_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
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
            .AddAttachment(Context->m_DepthAttachmentTexture)
            .Build();

        VkPushConstantRange pushConstant{};
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstant.offset = 0;
        pushConstant.size = sizeof(PushConstant);

        VkDescriptorSetLayout layouts[] =
        {
            VK::VulkanSceneRenderer::s_CameraDescriptorSetLayout.GetVkDescriptorSetLayout()
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = layouts;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

        if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
            PIX_ASSERT_MSG(false, "Failed to create pipeline layout!");

        auto bindingDesc = VulkanStaticMeshVertex::GetBindingDescription();
        auto attributeDesc = VulkanStaticMeshVertex::GetAttributeDescriptions();

        m_GraphicsPipeline.Init(Context->m_Device, m_Renderpass.GetVKRenderpass())
            .AddShaderStages(m_Shader.GetVertexShader(), m_Shader.GetFragmentShader())
            .AddVertexInputState(&bindingDesc, attributeDesc.data(), 1, attributeDesc.size())
            .AddViewportState(width, height)
            .AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
            .AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
            .AddDepthStencilState(true, true)
            .AddColorBlendState(true, 1)
            .SetPipelineLayout(m_PipelineLayout)
            .Build();
    }

    void FrostedGlassMaskRenderpass::Render(VulkanStaticMesh& mesh, const glm::mat4& transform)
    {
        auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
        auto specs = Engine::GetApplicationSpecs();

        VkClearValue clearValues[1] = {};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        //clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_Renderpass.GetVKRenderpass();
        renderPassInfo.framebuffer = m_Framebuffer.GetVKFramebuffer();
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { specs.Width, specs.Height };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VK::VulkanSceneRenderer::s_ImageIndex],
            &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VK::VulkanSceneRenderer::s_ImageIndex],
            VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline.GetVkPipeline());

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = (float)specs.Height;
        viewport.width = (float)specs.Width;
        viewport.height = -(float)specs.Height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { specs.Width, specs.Height };

        vkCmdSetViewport(VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VK::VulkanSceneRenderer::s_ImageIndex], 0, 1, &viewport);
        vkCmdSetScissor(VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VK::VulkanSceneRenderer::s_ImageIndex], 0, 1, &scissor);

        auto cameraDescriptorSet = VK::VulkanSceneRenderer::s_CameraDescriptorSets[VK::VulkanSceneRenderer::s_ImageIndex].GetVkDescriptorSet();
        vkCmdBindDescriptorSets(VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VK::VulkanSceneRenderer::s_ImageIndex],
            VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);

        VkBuffer vertexBuffers[] = { mesh.GetVertexBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VK::VulkanSceneRenderer::s_ImageIndex],
            0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VK::VulkanSceneRenderer::s_ImageIndex],
            mesh.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

        PushConstant pushData = { transform };
        vkCmdPushConstants(VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VK::VulkanSceneRenderer::s_ImageIndex],
            m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant), &pushData);

        for (const auto& subMesh : mesh.m_SubMeshes)
        {
            vkCmdDrawIndexed(VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VK::VulkanSceneRenderer::s_ImageIndex],
                subMesh.IndicesCount,
                1,
                subMesh.BaseIndex,
                subMesh.BaseVertex,
                0);
        }

        vkCmdEndRenderPass(VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[VK::VulkanSceneRenderer::s_ImageIndex]);
    }

    void FrostedGlassMaskRenderpass::OnResize(uint32_t width, uint32_t height)
    {
        auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

        vkDeviceWaitIdle(Context->m_Device);

        if (m_ColorAttachmentTexture) {
            m_ColorAttachmentTexture->Destroy();
            delete m_ColorAttachmentTexture;
        }
        m_ColorAttachmentTexture = new VK::VulkanTexture();
        m_ColorAttachmentTexture->CreateColorAttachment(width, height, VK_FORMAT_R16G16B16A16_SFLOAT);

        m_Framebuffer.Destroy();
        m_Framebuffer
            .Init(Context->m_Device, m_Renderpass.GetVKRenderpass(), width, height)
            .AddAttachment(m_ColorAttachmentTexture)
            .AddAttachment(Context->m_DepthAttachmentTexture)
            .Build();
    }

    void FrostedGlassMaskRenderpass::Destroy()
    {
        auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

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

        if (m_PipelineLayout) {
            vkDestroyPipelineLayout(Context->m_Device, m_PipelineLayout, nullptr);
            m_PipelineLayout = nullptr;
        }
    }
}
