#include "VulkanComputePipeline.h"
#include <Engine/Engine.hpp>
#include "VulkanHelper.h"

namespace PIX3D
{
    namespace VK
    {
        VulkanComputePipeline::~VulkanComputePipeline()
        {
            Cleanup();
        }

        void VulkanComputePipeline::Cleanup()
        {
            if (m_Pipeline != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(m_device, m_Pipeline, nullptr);
                m_Pipeline = VK_NULL_HANDLE;
            }
        }

        VulkanComputePipeline& VulkanComputePipeline::Init(VkDevice device)
        {
            m_device = device;
            m_Pipeline = VK_NULL_HANDLE;
            m_pipelineLayout = VK_NULL_HANDLE;

            // Initialize shader stage with default values
            m_shaderStage = {};
            m_shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            m_shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            m_shaderStage.pName = "main";  // Default entry point

            return *this;
        }

        VulkanComputePipeline& VulkanComputePipeline::AddComputeShader(VkShaderModule computeShader)
        {
            m_shaderStage.module = computeShader;
            return *this;
        }

        VulkanComputePipeline& VulkanComputePipeline::SetPipelineLayout(VkPipelineLayout layout)
        {
            m_pipelineLayout = layout;
            return *this;
        }

        VulkanComputePipeline& VulkanComputePipeline::SetWorkGroupSize(uint32_t x, uint32_t y, uint32_t z)
        {
            m_workGroupSizeX = x;
            m_workGroupSizeY = y;
            m_workGroupSizeZ = z;
            return *this;
        }

        void VulkanComputePipeline::Build()
        {
            PIX_ASSERT_MSG(m_shaderStage.module != VK_NULL_HANDLE, "Compute shader module must be set before building pipeline!");
            PIX_ASSERT_MSG(m_pipelineLayout != VK_NULL_HANDLE, "Pipeline layout must be set before building pipeline!");

            VkComputePipelineCreateInfo pipelineCreateInfo{};
            pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipelineCreateInfo.stage = m_shaderStage;
            pipelineCreateInfo.layout = m_pipelineLayout;

            // Create the compute pipeline
            if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
            {
                PIX_ASSERT_MSG(false, "Failed to create compute pipeline!");
            }
        }

        void VulkanComputePipeline::Run(VkDescriptorSet set, uint32_t work_groups_x, uint32_t work_groups_y, uint32_t work_groups_z)
        {
            auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

            // Create command buffer
            VkCommandBuffer commandBuffer = VulkanHelper::BeginSingleTimeCommands(Context->m_Device, Context->m_CommandPool);

            // Bind pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);

            // Bind descriptor set
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                m_pipelineLayout, 0, 1, &set, 0, nullptr);

            // Dispatch compute work
            vkCmdDispatch(commandBuffer, work_groups_x, work_groups_y, work_groups_z);

            // Submit command buffer
            VulkanHelper::EndSingleTimeCommands(Context->m_Device, Context->m_Queue.m_Queue, Context->m_CommandPool, commandBuffer);
        }

        void VulkanComputePipeline::RunWithBarrier(VkDescriptorSet set,
            uint32_t work_groups_x,
            uint32_t work_groups_y,
            uint32_t work_groups_z,
            const VkBufferMemoryBarrier& barrier)
        {
            auto* Context = static_cast<VK::VulkanGraphicsContext*>(Engine::GetGraphicsContext());

            // Create command buffer
            VkCommandBuffer commandBuffer = VulkanHelper::BeginSingleTimeCommands(Context->m_Device, Context->m_CommandPool);

            // Add pre-compute barrier
            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0,
                0, nullptr,
                1, &barrier,
                0, nullptr);

            // Bind pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);

            // Bind descriptor set
            vkCmdBindDescriptorSets(commandBuffer,
                VK_PIPELINE_BIND_POINT_COMPUTE,
                m_pipelineLayout,
                0, 1, &set,
                0, nullptr);

            // Dispatch compute work
            vkCmdDispatch(commandBuffer, work_groups_x, work_groups_y, work_groups_z);

            // Submit command buffer
            VulkanHelper::EndSingleTimeCommands(Context->m_Device,
                Context->m_Queue.m_Queue,
                Context->m_CommandPool,
                commandBuffer);
        }
    }
}
