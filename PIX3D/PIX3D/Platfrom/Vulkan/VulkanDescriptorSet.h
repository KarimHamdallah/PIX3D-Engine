#pragma once
#include "VulkanTexture.h"

namespace PIX3D
{
    namespace VK
    {
        class VulkanDescriptorSet
        {
        public:
            VulkanDescriptorSet& Init(VkDescriptorSetLayout layout);
            VulkanDescriptorSet& AddTexture(uint32_t binding, const VulkanTexture& texture);
            void Build();

            VkDescriptorSet GetVkDescriptorSet() const { return m_DescriptorSet; }

        private:
            VkDescriptorSetLayout m_DescriptorSetLayout;
            VkDescriptorSet m_DescriptorSet;
        };
    }
}
