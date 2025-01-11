#pragma once
#include "VulkanTexture.h"
#include "VulkanUniformBuffer.h"
#include "VulkanShaderStorageBuffer.h"
#include <unordered_map>

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
            void UpdateTexture(uint32_t binding, const VulkanTexture& texture);
        private:
            VkDescriptorSetLayout m_DescriptorSetLayout;
            VkDescriptorSet m_DescriptorSet;
            std::unordered_map<uint32_t, VkDescriptorBufferInfo> m_ShaderStorageBufferBindings;
            std::unordered_map<uint32_t, VkDescriptorBufferInfo> m_UniformBufferBindings;
            std::unordered_map<uint32_t, VkDescriptorImageInfo> m_ImageBindings;
        };
    }
}
