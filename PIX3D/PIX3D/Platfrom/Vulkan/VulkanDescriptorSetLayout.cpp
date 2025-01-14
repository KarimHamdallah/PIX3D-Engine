#include "VulkanDescriptorSetLayout.h"
#include <Engine/Engine.hpp>
#include <Core/Core.h>

namespace PIX3D
{
    namespace VK
    {
        VulkanDescriptorSetLayout& VulkanDescriptorSetLayout::AddBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t descriptorCount)
        {
            VkDescriptorSetLayoutBinding layoutBinding = {};
            layoutBinding.binding = binding;
            layoutBinding.descriptorType = type;
            layoutBinding.stageFlags = stageFlags;
            layoutBinding.descriptorCount = descriptorCount;
            layoutBinding.pImmutableSamplers = nullptr;
            
            m_Bindings.push_back(layoutBinding);
            
            return *this;
        }

        void VulkanDescriptorSetLayout::Build()
        {
            auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkDescriptorSetLayoutCreateInfo layoutInfo = {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(m_Bindings.size());
            layoutInfo.pBindings = m_Bindings.data();

            if (vkCreateDescriptorSetLayout(Context->m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
                PIX_ASSERT_MSG(false, "Failed to create descriptor set layout!");
        }

        void VulkanDescriptorSetLayout::Destroy()
        {
            if (m_DescriptorSetLayout != VK_NULL_HANDLE)
            {
                auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
                vkDestroyDescriptorSetLayout(Context->m_Device, m_DescriptorSetLayout, nullptr);
                m_DescriptorSetLayout = VK_NULL_HANDLE;
                m_Bindings.clear();
            }
        }
    }
}
