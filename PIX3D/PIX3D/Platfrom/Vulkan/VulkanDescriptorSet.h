#pragma once
#include "VulkanTexture.h"
#include "VulkanUniformBuffer.h"
#include "VulkanShaderStorageBuffer.h"

namespace PIX3D
{
    namespace VK
    {
        class VulkanDescriptorSet
        {
        public:
            VulkanDescriptorSet& Init(VkDescriptorSetLayout layout);
            VulkanDescriptorSet& AddTexture(uint32_t binding, const VulkanTexture& texture);
            VulkanDescriptorSet& AddUniformBuffer(uint32_t binding, const VulkanUniformBuffer& uniformBuffer);
            VulkanDescriptorSet& AddShaderStorageBuffer(uint32_t binding, const VulkanShaderStorageBuffer& storageBuffer);
            void Build();

            VkDescriptorSet GetVkDescriptorSet() const { return m_DescriptorSet; }

        private:
            VkDescriptorSetLayout m_DescriptorSetLayout;
            VkDescriptorSet m_DescriptorSet;

            std::vector<VkDescriptorBufferInfo> m_BufferInfos; // Stores buffer infos
            std::vector<VkDescriptorImageInfo> m_ImageInfos;   // Stores image infos
        };
    }
}
