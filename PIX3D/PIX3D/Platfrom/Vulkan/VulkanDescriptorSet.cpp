#include "VulkanDescriptorSet.h"
#include <Engine/Engine.hpp>

namespace PIX3D
{
    namespace VK
    {
        VulkanDescriptorSet& VulkanDescriptorSet::Init(VkDescriptorSetLayout layout)
        {
            m_DescriptorSetLayout = layout;
            auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = Context->m_DescriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &m_DescriptorSetLayout;

            if (vkAllocateDescriptorSets(Context->m_Device, &allocInfo, &m_DescriptorSet) != VK_SUCCESS)
                PIX_ASSERT_MSG(false, "Failed to allocate descriptor set!")
            return *this;
        }

        void VulkanDescriptorSet::Build()
        {
            auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

            std::vector<VkWriteDescriptorSet> descriptorWrites;

            // Construct buffer descriptor writes
            for (size_t i = 0; i < m_BufferInfos.size(); ++i)
            {
                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = m_DescriptorSet;
                descriptorWrite.dstBinding = static_cast<uint32_t>(i); // Use appropriate binding logic here
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = &m_BufferInfos[i];

                descriptorWrites.push_back(descriptorWrite);
            }

            // Construct image descriptor writes
            for (size_t i = 0; i < m_ImageInfos.size(); ++i)
            {
                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = m_DescriptorSet;
                descriptorWrite.dstBinding = static_cast<uint32_t>(m_BufferInfos.size() + i); // Use appropriate binding logic here
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pImageInfo = &m_ImageInfos[i];

                descriptorWrites.push_back(descriptorWrite);
            }

            // Update descriptor sets
            vkUpdateDescriptorSets(Context->m_Device,
                static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(),
                0,
                nullptr);
        }

        VulkanDescriptorSet& VulkanDescriptorSet::AddTexture(uint32_t binding, const VulkanTexture& texture)
        {
            // Add image info to persistent storage

            m_ImageInfos.emplace_back(VkDescriptorImageInfo{});
            VkDescriptorImageInfo& imageInfo = m_ImageInfos.back();
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture.GetImageView();
            imageInfo.sampler = texture.GetSampler();

            return *this;
        }

        VulkanDescriptorSet& VulkanDescriptorSet::AddUniformBuffer(uint32_t binding, const VulkanUniformBuffer& uniformBuffer)
        {
            // Add buffer info to persistent storage

            m_BufferInfos.emplace_back(VkDescriptorBufferInfo{});
            VkDescriptorBufferInfo& bufferInfo = m_BufferInfos.back();
            bufferInfo.buffer = uniformBuffer.GetBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = uniformBuffer.GetSize();

            return *this;
        }

        VulkanDescriptorSet& VulkanDescriptorSet::AddShaderStorageBuffer(uint32_t binding, const VulkanShaderStorageBuffer& storageBuffer)
        {
            // Add buffer info to persistent storage

            m_BufferInfos.emplace_back(VkDescriptorBufferInfo{});
            VkDescriptorBufferInfo& bufferInfo = m_BufferInfos.back();
            bufferInfo.buffer = storageBuffer.GetBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = storageBuffer.GetSize();

            return *this;
        }
    }
}
