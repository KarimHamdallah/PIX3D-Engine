#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace PIX3D
{
    namespace VK
    {
        class VulkanDescriptorSetLayout
        {
        public:
            VulkanDescriptorSetLayout& AddBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1);
            void Build();

            operator const VkDescriptorSetLayout() const
            {
                return m_DescriptorSetLayout;
            }

            VkDescriptorSetLayout GetVkDescriptorSetLayout() { return m_DescriptorSetLayout; }

        private:
            std::vector<VkDescriptorSetLayoutBinding> m_Bindings;
            VkDescriptorSetLayout m_DescriptorSetLayout;
        };
    }
}
