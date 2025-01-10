#include "VulkanGraphicsPipeline.h"
#include <Core/Core.h>

namespace PIX3D
{
    namespace VK
    {
        VulkanGraphicsPipeline& VulkanGraphicsPipeline::Init(VkDevice device, VkRenderPass renderPass)
        {
            m_device = device;
            m_renderPass = renderPass;
            m_pipelineLayout = VK_NULL_HANDLE;

            // Initialize m_vertexInputState
            m_vertexInputState = {};
            m_vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            m_vertexInputState.vertexBindingDescriptionCount = 0;
            m_vertexInputState.pVertexBindingDescriptions = nullptr;
            m_vertexInputState.vertexAttributeDescriptionCount = 0;
            m_vertexInputState.pVertexAttributeDescriptions = nullptr;

            // Initialize m_inputAssemblyState
            m_inputAssemblyState = {};
            m_inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            m_inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            m_inputAssemblyState.primitiveRestartEnable = VK_FALSE;

            // Initialize m_viewportState
            m_viewportState = {};
            m_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            m_viewportState.viewportCount = 1; // Default is one viewport
            m_viewportState.pViewports = nullptr; // Viewports are set during building
            m_viewportState.scissorCount = 1;
            m_viewportState.pScissors = nullptr; // Scissors are set during building

            // Initialize m_rasterizationState
            m_rasterizationState = {};
            m_rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            m_rasterizationState.depthClampEnable = VK_FALSE;
            m_rasterizationState.rasterizerDiscardEnable = VK_FALSE;
            m_rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
            m_rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
            m_rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
            m_rasterizationState.depthBiasEnable = VK_FALSE;
            m_rasterizationState.lineWidth = 1.0f;

            // Initialize m_multisampleState
            m_multisampleState = {};
            m_multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            m_multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            m_multisampleState.sampleShadingEnable = VK_FALSE;

            m_depthStencilState = {};
            m_depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            m_depthStencilState.depthTestEnable = VK_TRUE;
            m_depthStencilState.depthWriteEnable = VK_TRUE;
            m_depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
            m_depthStencilState.depthBoundsTestEnable = VK_FALSE;
            m_depthStencilState.stencilTestEnable = VK_FALSE;

            m_colorBlendState = {};

            return *this;
        }

        // Add shader stages
        VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddShaderStages(VkShaderModule vertexShader, VkShaderModule fragmentShader)
        {
            VkPipelineShaderStageCreateInfo vertexShaderStage = {};
            vertexShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertexShaderStage.module = vertexShader;
            vertexShaderStage.pName = "main";
            m_shaderStages.push_back(vertexShaderStage);

            VkPipelineShaderStageCreateInfo fragmentShaderStage = {};
            fragmentShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragmentShaderStage.module = fragmentShader;
            fragmentShaderStage.pName = "main";
            m_shaderStages.push_back(fragmentShaderStage);

            return *this;
        }

        // Add viewport and scissor state
        VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddViewportState(float width, float height)
        {
            static VkViewport viewport = {};
            viewport.x = 0.0f;
            viewport.y = height;
            viewport.width = width;
            viewport.height = -height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            static VkRect2D scissor = {};
            scissor.offset = { 0, 0 };
            scissor.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

            m_viewportState = {};
            m_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            m_viewportState.viewportCount = 1;
            m_viewportState.pViewports = &viewport;
            m_viewportState.scissorCount = 1;
            m_viewportState.pScissors = &scissor;

            return *this;
        }

        // Add vertex input state
        VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddVertexInputState(
            const VkVertexInputBindingDescription* pBindingDescription,
            const VkVertexInputAttributeDescription* pAttributeDescriptions,
            uint32_t bindingCount,
            uint32_t attributeCount)
        {
            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

            if (pBindingDescription && bindingCount > 0)
            {
                vertexInputInfo.vertexBindingDescriptionCount = bindingCount;
                vertexInputInfo.pVertexBindingDescriptions = pBindingDescription;
            }
            else
            {
                vertexInputInfo.vertexBindingDescriptionCount = 0;
                vertexInputInfo.pVertexBindingDescriptions = nullptr;
            }

            if (pAttributeDescriptions && attributeCount > 0)
            {
                vertexInputInfo.vertexAttributeDescriptionCount = attributeCount;
                vertexInputInfo.pVertexAttributeDescriptions = pAttributeDescriptions;
            }
            else
            {
                vertexInputInfo.vertexAttributeDescriptionCount = 0;
                vertexInputInfo.pVertexAttributeDescriptions = nullptr;
            }

            m_vertexInputState = vertexInputInfo;
            return *this;
        }

        // Add input assembly state
        VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddInputAssemblyState(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable)
        {
            m_inputAssemblyState = {};
            m_inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            m_inputAssemblyState.topology = topology;
            m_inputAssemblyState.primitiveRestartEnable = primitiveRestartEnable;

            return *this;
        }

        // Add rasterization state
        VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddRasterizationState(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace)
        {
            m_rasterizationState = {};
            m_rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            m_rasterizationState.polygonMode = polygonMode;
            m_rasterizationState.cullMode = cullMode;
            m_rasterizationState.frontFace = frontFace;
            m_rasterizationState.lineWidth = 1.0f;

            return *this;
        }

        // Add multisample state
        VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddMultisampleState(VkSampleCountFlagBits rasterizationSamples)
        {
            m_multisampleState = {};
            m_multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            m_multisampleState.rasterizationSamples = rasterizationSamples;

            return *this;
        }

        // Add color blend state
        VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddColorBlendState(bool enable_blend, uint32_t attachmentCount)
        {
            // Clear previous attachments
            m_colorBlendAttachments.clear();

            // Create and add the attachment states
            for (uint32_t i = 0; i < attachmentCount; ++i)
            {
                VkPipelineColorBlendAttachmentState blendAttachState = {};
                blendAttachState.blendEnable = enable_blend ? VK_TRUE : VK_FALSE;
                blendAttachState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;  // Default valid value
                blendAttachState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Default valid value
                blendAttachState.colorBlendOp = VK_BLEND_OP_ADD;            // Default valid value
                blendAttachState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Default valid value
                blendAttachState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Default valid value
                blendAttachState.alphaBlendOp = VK_BLEND_OP_ADD;             // Default valid value
                blendAttachState.colorWriteMask =
                    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

                m_colorBlendAttachments.push_back(blendAttachState);
            }

            m_colorBlendState =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable = VK_FALSE,
                .logicOp = VK_LOGIC_OP_COPY,
                .attachmentCount = attachmentCount,
                .pAttachments = m_colorBlendAttachments.data()  // Use internal attachment states
            };

            return *this;
        }

        VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddDepthStencilState(bool depthTestEnable, bool depthWriteEnable)
        {
            m_depthStencilState = {};
            m_depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            m_depthStencilState.depthTestEnable = depthTestEnable ? VK_TRUE : VK_FALSE;
            m_depthStencilState.depthWriteEnable = depthWriteEnable ? VK_TRUE : VK_FALSE;
            m_depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;  // Standard depth testing
            m_depthStencilState.depthBoundsTestEnable = VK_FALSE;
            m_depthStencilState.stencilTestEnable = VK_FALSE;
            m_depthStencilState.minDepthBounds = 0.0f;
            m_depthStencilState.maxDepthBounds = 1.0f;

            return *this;
        }

        // Set pipeline layout
        VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetPipelineLayout(VkPipelineLayout layout)
        {
            m_pipelineLayout = layout;
            return *this;
        }

        // Build the Vulkan pipeline
        void VulkanGraphicsPipeline::Build()
        {
            VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
            pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineCreateInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
            pipelineCreateInfo.pStages = m_shaderStages.data();
            pipelineCreateInfo.pVertexInputState = &m_vertexInputState;
            pipelineCreateInfo.pInputAssemblyState = &m_inputAssemblyState;
            pipelineCreateInfo.pViewportState = &m_viewportState;
            pipelineCreateInfo.pRasterizationState = &m_rasterizationState;
            pipelineCreateInfo.pMultisampleState = &m_multisampleState;
            pipelineCreateInfo.pDepthStencilState = &m_depthStencilState;
            pipelineCreateInfo.pColorBlendState = &m_colorBlendState;
            pipelineCreateInfo.layout = m_pipelineLayout;
            pipelineCreateInfo.renderPass = m_renderPass;
            pipelineCreateInfo.subpass = 0;

            if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
                PIX_ASSERT_MSG(false, "Failed to create graphics pipeline!");
        }
    }
}
