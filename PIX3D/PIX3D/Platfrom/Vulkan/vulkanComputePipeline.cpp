#include "vulkanComputePipeline.h"
#include <Core/Core.h>

namespace PIX3D
{
    namespace VK
    {

        VulkanCompute::VulkanCompute(VkDevice device, uint32_t computeQueueFamilyIndex)
            : m_Device(device)
            , m_ComputeQueueFamilyIndex(computeQueueFamilyIndex)
            , m_ComputePipeline(VK_NULL_HANDLE)
            , m_PipelineLayout(VK_NULL_HANDLE)
            , m_DescriptorPool(VK_NULL_HANDLE)
            , m_CommandPool(VK_NULL_HANDLE)
        {
        }

        void VulkanCompute::Init()
        {
            vkGetDeviceQueue(m_Device, m_ComputeQueueFamilyIndex, 0, &m_ComputeQueue);
            CreateCommandPool();
        }

        void VulkanCompute::CreateCommandPool()
        {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex = m_ComputeQueueFamilyIndex;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
                PIX_ASSERT_MSG(false, "Failed to create command pool!");
        }

        void VulkanCompute::AddDescriptorSetLayout(const std::vector<ComputeDescriptorSetBinding>& bindings)
        {
            std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
            for (const auto& binding : bindings)
            {
                VkDescriptorSetLayoutBinding layoutBinding{};
                layoutBinding.binding = binding.binding;
                layoutBinding.descriptorType = binding.type;
                layoutBinding.descriptorCount = binding.count;
                layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                layoutBindings.push_back(layoutBinding);
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
            layoutInfo.pBindings = layoutBindings.data();

            VkDescriptorSetLayout layout;
            if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
                PIX_ASSERT_MSG(false, "Failed to create descriptor set layout!");

            m_DescriptorSetLayouts.push_back(layout);
            m_DescriptorSetBindings.push_back(bindings);
        }

        void VulkanCompute::CreatePipelineLayout()
        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_DescriptorSetLayouts.size());
            pipelineLayoutInfo.pSetLayouts = m_DescriptorSetLayouts.data();

            if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
                PIX_ASSERT_MSG(false, "Failed to create pipeline layout");
        }

        void VulkanCompute::CreatePipeline(const std::vector<uint32_t>& shaderCode)
        {
            CreatePipelineLayout();
            CreateComputePipeline(shaderCode);
            CreateDescriptorPool();
            AllocateDescriptorSets();
        }

        void VulkanCompute::CreateDescriptorPool()
        {
            std::map<VkDescriptorType, uint32_t> typeCount;

            for (const auto& setBinding : m_DescriptorSetBindings)
            {
                for (const auto& binding : setBinding)
                {
                    typeCount[binding.type] += binding.count;
                }
            }

            std::vector<VkDescriptorPoolSize> poolSizes;

            for (const auto& [type, count] : typeCount)
            {
                VkDescriptorPoolSize poolSize{};
                poolSize.type = type;
                poolSize.descriptorCount = count;
                poolSizes.push_back(poolSize);
            }

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.maxSets = static_cast<uint32_t>(m_DescriptorSetLayouts.size());
            poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            poolInfo.pPoolSizes = poolSizes.data();

            if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
                PIX_ASSERT_MSG(false, "Failed to create descriptor pool!");
        }


        void VulkanCompute::UpdateDescriptorSet(uint32_t setIndex, const std::vector<std::pair<uint32_t, VkBuffer>>& bufferBindings)
        {
            if (setIndex >= m_DescriptorSets.size())
                PIX_ASSERT_MSG(false, "Invalid descriptor set index!");

            std::vector<VkDescriptorBufferInfo> bufferInfos;
            std::vector<VkWriteDescriptorSet> descriptorWrites;

            for (const auto& [binding, buffer] : bufferBindings)
            {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = buffer;
                bufferInfo.offset = 0;
                bufferInfo.range = VK_WHOLE_SIZE;
                bufferInfos.push_back(bufferInfo);

                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = m_DescriptorSets[setIndex];
                descriptorWrite.dstBinding = binding;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = &bufferInfos.back();
                descriptorWrites.push_back(descriptorWrite);
            }

            vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

        void VulkanCompute::Dispatch(uint32_t x, uint32_t y, uint32_t z)
        {
            VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);

            if (!m_DescriptorSets.empty())
            {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, static_cast<uint32_t>(m_DescriptorSets.size()), m_DescriptorSets.data(), 0, nullptr);
            }

            vkCmdDispatch(commandBuffer, x, y, z);

            EndSingleTimeCommands(commandBuffer);
        }
    }
}
