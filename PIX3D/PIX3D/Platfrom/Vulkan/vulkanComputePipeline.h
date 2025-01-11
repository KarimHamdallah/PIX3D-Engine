#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace PIX3D
{
    namespace VK
    {
        class VulkanComputePipeline
        {
        public:
            VulkanComputePipeline() = default;
            ~VulkanComputePipeline();

            // Initialize with device
            VulkanComputePipeline& Init(VkDevice device);

            // Add compute shader stage
            VulkanComputePipeline& AddComputeShader(VkShaderModule computeShader);

            // Set pipeline layout (for descriptor sets and push constants)
            VulkanComputePipeline& SetPipelineLayout(VkPipelineLayout layout);

            // Set workgroup size for the compute shader
            VulkanComputePipeline& SetWorkGroupSize(uint32_t x, uint32_t y, uint32_t z);

            // Build the pipeline
            void Build();

            // Get the pipeline handle
            VkPipeline GetPipeline() const { return m_Pipeline; }

            // Get the optimal work group size
            void GetWorkGroupSize(uint32_t& x, uint32_t& y, uint32_t& z) const
            {
                x = m_workGroupSizeX;
                y = m_workGroupSizeY;
                z = m_workGroupSizeZ;
            }

            void Run(VkDescriptorSet set, uint32_t work_groups_x, uint32_t work_groups_y, uint32_t work_groups_z);

            void RunWithBarrier(VkDescriptorSet set,
                uint32_t work_groups_x,
                uint32_t work_groups_y,
                uint32_t work_groups_z,
                const VkBufferMemoryBarrier& barrier);

        private:
            VkDevice m_device = VK_NULL_HANDLE;
            VkPipeline m_Pipeline = VK_NULL_HANDLE;
            VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

            // Shader stage
            VkPipelineShaderStageCreateInfo m_shaderStage{};

            // Work group sizes
            uint32_t m_workGroupSizeX = 1;
            uint32_t m_workGroupSizeY = 1;
            uint32_t m_workGroupSizeZ = 1;

            void Cleanup();
        };
    }
}
