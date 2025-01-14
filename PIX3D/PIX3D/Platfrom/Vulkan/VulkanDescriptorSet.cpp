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

            // Add shader storage buffer descriptors
            for (const auto& [binding, bufferInfo] : m_ShaderStorageBufferBindings)
            {
                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = m_DescriptorSet;
                descriptorWrite.dstBinding = binding;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = &bufferInfo;

                descriptorWrites.push_back(descriptorWrite);
            }

            // Add uniform buffer descriptors
            for (const auto& [binding, bufferInfo] : m_UniformBufferBindings)
            {
                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = m_DescriptorSet;
                descriptorWrite.dstBinding = binding;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = &bufferInfo;

                descriptorWrites.push_back(descriptorWrite);
            }

            // Add image descriptors
            for (const auto& [binding, imageInfo] : m_ImageBindings)
            {
                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = m_DescriptorSet;
                descriptorWrite.dstBinding = binding;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pImageInfo = &imageInfo;

                descriptorWrites.push_back(descriptorWrite);
            }

            vkUpdateDescriptorSets(Context->m_Device,
                static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(),
                0,
                nullptr);
        }

        VulkanDescriptorSet& VulkanDescriptorSet::AddTexture(uint32_t binding, const VulkanTexture& texture)
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture.GetImageView();
            imageInfo.sampler = texture.GetSampler();

            m_ImageBindings[binding] = imageInfo;
            return *this;
        }

        VulkanDescriptorSet& VulkanDescriptorSet::AddTexture(uint32_t binding, VkImageView ImageView, VkSampler Sampler)
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = ImageView;
            imageInfo.sampler = Sampler;

            m_ImageBindings[binding] = imageInfo;
            return *this;
        }

        VulkanDescriptorSet& VulkanDescriptorSet::AddCubemap(uint32_t binding, const VulkanHdrCubemap& cubemap)
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = cubemap.m_ImageView;
            imageInfo.sampler = cubemap.m_Sampler;

            m_ImageBindings[binding] = imageInfo;
            return *this;
        }

        VulkanDescriptorSet& VulkanDescriptorSet::AddCubemap(uint32_t binding, VkImageView ImageView, VkSampler Sampler)
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = ImageView;
            imageInfo.sampler = Sampler;

            m_ImageBindings[binding] = imageInfo;
            return *this;
        }

        VulkanDescriptorSet& VulkanDescriptorSet::AddUniformBuffer(uint32_t binding, const VulkanUniformBuffer& uniformBuffer)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffer.GetBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = uniformBuffer.GetSize();

            m_UniformBufferBindings[binding] = bufferInfo;
            return *this;
        }

        VulkanDescriptorSet& VulkanDescriptorSet::AddShaderStorageBuffer(uint32_t binding, const VulkanShaderStorageBuffer& storageBuffer)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = storageBuffer.GetBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = storageBuffer.GetSize();

            m_ShaderStorageBufferBindings[binding] = bufferInfo;
            return *this;
        }

        void VulkanDescriptorSet::UpdateTexture(uint32_t binding, const VulkanTexture& texture)
        {
            auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture.GetImageView();
            imageInfo.sampler = texture.GetSampler();

            m_ImageBindings[binding] = imageInfo;

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_DescriptorSet;
            descriptorWrite.dstBinding = binding;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &m_ImageBindings[binding];

            vkUpdateDescriptorSets(Context->m_Device, 1, &descriptorWrite, 0, nullptr);
        }

        void VulkanDescriptorSet::Destroy()
        {
            if (m_DescriptorSet != VK_NULL_HANDLE)
            {
                auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

                // Free the descriptor set
                vkFreeDescriptorSets(Context->m_Device, Context->m_DescriptorPool, 1, &m_DescriptorSet);
                m_DescriptorSet = VK_NULL_HANDLE;

                // Clear bindings maps
                m_ShaderStorageBufferBindings.clear();
                m_UniformBufferBindings.clear();
                m_ImageBindings.clear();
            }
        }
    }
}
