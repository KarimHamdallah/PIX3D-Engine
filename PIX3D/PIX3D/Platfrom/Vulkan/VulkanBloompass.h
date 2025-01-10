#pragma once
#include "VulkanRenderpass.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanDescriptorSet.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanFramebuffer.h"
#include "VulkanTexture.h"
#include "VulkanShader.h"
#include "VulkanStaticMeshFactory.h"
#include <glm/glm.hpp>

namespace PIX3D
{
    namespace VK
    {
        struct BloomPushConstant  // Replace BloomUBO
        {
            glm::vec2 direction;
            int mipLevel;
            int useInputTexture;
        };

        class VulkanBloomPass
        {
        public:
            void Init(uint32_t width, uint32_t height, VulkanTexture* bloom_brightness_texture);
            void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t numIterations = 2);
            VulkanTexture* GetFinalBloomTexture() { return m_BloomTextures[m_FinalResultBufferIndex]; }
            void Destroy();
            void Resize(uint32_t width, uint32_t height);

        private:
            void SetupFramebuffers(int mipLevel);

            uint32_t m_Width = 0;
            uint32_t m_Height = 0;
            
            enum
            {
                MAX_MIP_LEVELS = 6
            };

            VulkanShader m_BloomShader;
            VulkanStaticMeshData m_QuadMesh;

            VulkanDescriptorSetLayout m_DescriptorSetLayout;
            VulkanDescriptorSet m_DescriptorSets[2];

            VulkanRenderPass m_Renderpasses[2];
            std::vector<VulkanFramebuffer> m_Framebuffers[2];  // One vector per ping-pong buffer
            VkPipelineLayout m_PipelineLayouts[2];
            VulkanGraphicsPipeline m_Pipelines[2];

            VulkanTexture* m_BloomTextures[2];  // For ping-pong blurring
            VulkanTexture* m_InputTexture = nullptr;

            uint32_t m_FinalResultBufferIndex = 0;
        };
    }
}
