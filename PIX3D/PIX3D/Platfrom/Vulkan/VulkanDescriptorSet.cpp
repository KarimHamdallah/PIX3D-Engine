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
        }

        VulkanDescriptorSet& VulkanDescriptorSet::AddTexture(uint32_t binding, const VulkanTexture& texture)
        {
            auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture.GetImageView();
            imageInfo.sampler = texture.GetSampler();

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_DescriptorSet;
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(Context->m_Device, 1, &descriptorWrite, 0, nullptr);

            return *this;
        }
    }
}
