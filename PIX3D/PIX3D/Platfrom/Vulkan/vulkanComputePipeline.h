#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <map>

namespace PIX3D
{
    namespace VK
    {
        struct ComputeDescriptorSetBinding
        {
            uint32_t binding;
            VkDescriptorType type;
            uint32_t count;
        };

        class VulkanCompute
        {
        public:
            VulkanCompute(VkDevice device, uint32_t computeQueueFamilyIndex);
            ~VulkanCompute() = default;

            void Init();

            VulkanCompute(const VulkanCompute&) = delete;
            VulkanCompute& operator=(const VulkanCompute&) = delete;

            void AddDescriptorSetLayout(const std::vector<ComputeDescriptorSetBinding>& bindings);
            void CreatePipeline(const std::vector<uint32_t>& shaderCode);
            void UpdateDescriptorSet(uint32_t setIndex, const std::vector<std::pair<uint32_t, VkBuffer>>& bufferBindings);
            void Dispatch(uint32_t x, uint32_t y, uint32_t z);

        private:
            VkDevice m_Device;
            VkPipeline m_ComputePipeline;
            VkPipelineLayout m_PipelineLayout;
            std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
            VkDescriptorPool m_DescriptorPool;
            std::vector<VkDescriptorSet> m_DescriptorSets;
            VkCommandPool m_CommandPool;
            VkQueue m_ComputeQueue;
            uint32_t m_ComputeQueueFamilyIndex;

            std::vector<std::vector<ComputeDescriptorSetBinding>> m_DescriptorSetBindings;

        private:
            void CreateCommandPool();
            void CreatePipelineLayout();
            void CreateDescriptorPool();
            void AllocateDescriptorSets();
            void CreateComputePipeline(const std::vector<uint32_t>& shaderCode);
            VkCommandBuffer BeginSingleTimeCommands();
            void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
            void Cleanup();
        };
    }
}
