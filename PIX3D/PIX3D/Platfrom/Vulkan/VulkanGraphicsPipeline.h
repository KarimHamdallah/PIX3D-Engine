#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace PIX3D
{
    namespace VK
    {
        class VulkanGraphicsPipeline
        {
        public:
            VulkanGraphicsPipeline() = default;
            ~VulkanGraphicsPipeline() {}

            VulkanGraphicsPipeline& Init(VkDevice device, VkRenderPass renderPass);

            // Configuration methods for specific pipeline stages
            VulkanGraphicsPipeline& AddShaderStages(VkShaderModule vertexShader, VkShaderModule fragmentShader);
            VulkanGraphicsPipeline& AddViewportState(float width, float height);
            VulkanGraphicsPipeline& AddVertexInputState(
                const VkVertexInputBindingDescription* pBindingDescription,
                const VkVertexInputAttributeDescription* pAttributeDescriptions,
                uint32_t bindingCount,
                uint32_t attributeCount);
            VulkanGraphicsPipeline& AddInputAssemblyState(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable);
            VulkanGraphicsPipeline& AddRasterizationState(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace);
            VulkanGraphicsPipeline& AddMultisampleState(VkSampleCountFlagBits rasterizationSamples);
            VulkanGraphicsPipeline& AddColorBlendState(bool enable_blend, uint32_t attachmentCount);
            VulkanGraphicsPipeline& AddDepthStencilState(bool depthTestEnable = true, bool depthWriteEnable = true, VkCompareOp compareOperation = VK_COMPARE_OP_LESS);
            VulkanGraphicsPipeline& SetPipelineLayout(VkPipelineLayout layout);

            void Build();

            VkPipeline GetVkPipeline() { return m_Pipeline; }

            void Destroy();

        private:
            VkDevice m_device = nullptr;
            VkRenderPass m_renderPass = nullptr;
            VkPipelineLayout m_pipelineLayout = nullptr;

            VkPipeline m_Pipeline = nullptr;

            // Pipeline state objects
            std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
            VkPipelineVertexInputStateCreateInfo m_vertexInputState;
            VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyState;
            VkPipelineViewportStateCreateInfo m_viewportState;
            VkPipelineRasterizationStateCreateInfo m_rasterizationState;
            VkPipelineMultisampleStateCreateInfo m_multisampleState;
            VkPipelineColorBlendStateCreateInfo m_colorBlendState;
            VkPipelineDepthStencilStateCreateInfo m_depthStencilState;

            std::vector<VkPipelineColorBlendAttachmentState> m_colorBlendAttachments;

            VkPipelineDynamicStateCreateInfo m_dynamicState;
            std::vector<VkDynamicState> m_dynamicStates;
        };
    }
}
