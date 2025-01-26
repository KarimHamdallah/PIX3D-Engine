#pragma once
#include <Platfrom/Vulkan/VulkanComputePipeline.h>
#include <Platfrom/Vulkan/VulkanTexture.h>
#include <Platfrom/Vulkan/VulkanDescriptorSetLayout.h>
#include <Platfrom/Vulkan/VulkanDescriptorSet.h>
#include <Platfrom/Vulkan/VulkanShader.h>

namespace PIX3D::VK
{
    class FrostedGlassBlurPass
    {
    public:
        void Init(uint32_t width, uint32_t height);
        void Process(VulkanTexture* inputTexture);
        void Destroy();

        VulkanTexture* GetOutputTexture() { return m_OutputTexture; }

    private:
        struct Settings {
            int width;
            int height;
            float _pad1;
            float _pad2;
        };

        VulkanShader m_ComputeShader;
        VulkanComputePipeline m_Pipeline;
        VkPipelineLayout m_PipelineLayout = nullptr;

        VulkanTexture* m_OutputTexture = nullptr;
        VulkanDescriptorSetLayout m_DescriptorSetLayout;
        VulkanDescriptorSet m_DescriptorSet;

        VulkanShaderStorageBuffer m_SettingsBuffer;
        Settings m_Settings;

        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
    };
}
