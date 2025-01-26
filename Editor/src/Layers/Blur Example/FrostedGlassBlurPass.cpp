#include "FrostedGlassBlurPass.h"
#include <Engine/Engine.hpp>

namespace PIX3D::VK
{
    void FrostedGlassBlurPass::Init(uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;
        auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

        m_OutputTexture = new VulkanTexture();
        m_OutputTexture->CreateColorAttachment(width, height, VK_FORMAT_R16G16B16A16_SFLOAT, 1, false);

        // transition from undefined to general
        VulkanTextureHelper::TransitionImageLayout(
            m_OutputTexture->GetImage(),
            m_OutputTexture->GetFormat(),
            0, 1,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        m_ComputeShader.LoadComputeShaderFromFile("../PIX3D/res/vk shaders/Frosted Glass Example/frostedglass_blur.comp");

        m_DescriptorSetLayout
            .AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .Build();

        m_Settings = {
            .width = (int)width,
            .height = (int)height,
            ._pad1 = 0.0f,
            ._pad2 = 0.0f
        };
        m_SettingsBuffer.Create(sizeof(Settings));
        m_SettingsBuffer.UpdateData(&m_Settings, sizeof(Settings));

        VkDescriptorSetLayout layouts[] =
        {
            m_DescriptorSetLayout.GetVkDescriptorSetLayout()
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = layouts;

        if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
            PIX_ASSERT_MSG(false, "Failed to create compute pipeline layout!");

        m_Pipeline.Init(Context->m_Device)
            .AddComputeShader(m_ComputeShader.GetComputeShader())
            .SetPipelineLayout(m_PipelineLayout)
            .SetWorkGroupSize(16, 16, 1)
            .Build();
    }

    void FrostedGlassBlurPass::Process(VulkanTexture* inputTexture)
    {
        auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

        VulkanTextureHelper::TransitionImageLayout(
            m_OutputTexture->GetImage(),
            m_OutputTexture->GetFormat(),
            0, 1,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL
        );


        if (!m_DescriptorSet.GetVkDescriptorSet())
        {
            m_DescriptorSet
                .Init(m_DescriptorSetLayout)
                .AddShaderStorageBuffer(0, m_SettingsBuffer)
                .AddStorageTexture(1, *inputTexture)
                .AddStorageTexture(2, *m_OutputTexture)
                .Build();
        }

        uint32_t workGroupsX = (m_Width + 15) / 16;
        uint32_t workGroupsY = (m_Height + 15) / 16;
        m_Pipeline.Run(m_DescriptorSet.GetVkDescriptorSet(), workGroupsX, workGroupsY, 1);


        VulkanTextureHelper::TransitionImageLayout(
            m_OutputTexture->GetImage(),
            m_OutputTexture->GetFormat(),
            0, 1,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
    }

    void FrostedGlassBlurPass::Destroy()
    {
        auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

        if (m_OutputTexture) {
            m_OutputTexture->Destroy();
            delete m_OutputTexture;
            m_OutputTexture = nullptr;
        }

        m_ComputeShader.Destroy();
        m_DescriptorSetLayout.Destroy();
        m_DescriptorSet.Destroy();
        m_SettingsBuffer.Destroy();

        if (m_PipelineLayout) {
            vkDestroyPipelineLayout(Context->m_Device, m_PipelineLayout, nullptr);
            m_PipelineLayout = nullptr;
        }
    }
}
