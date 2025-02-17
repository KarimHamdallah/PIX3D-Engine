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
        enum VKBloomDirection
        {
            BOTH = 0,
            HORIZONTAL,
            VERTICAL
        };

        struct BloomPushConstant
        {
            glm::vec2 direction;
            int mipLevel;
            int useInputTexture;
        };

        class VulkanBloomPass
        {
        public:
            void Init(uint32_t width, uint32_t height);
            void RecordCommandBuffer(VulkanTexture* bloom_brightness_texture, VkCommandBuffer commandBuffer);
            VulkanTexture* GetFinalBloomTexture() { return m_ColorAttachments[m_FinalResultBufferIndex]; }
            void Destroy();
            void OnResize(uint32_t width, uint32_t height);

        public:
            int m_BloomDirection = VKBloomDirection::BOTH;
            int m_NumIterations = 10;
        private:
            void SetupFramebuffers(int mipLevel);

            uint32_t m_Width = 0;
            uint32_t m_Height = 0;
            
            enum
            {
                BLUR_DOWN_SAMPLES = 6,
                BLUR_BUFFERS_COUNT = 2,
                HORIZONTAL_BLUR_BUFFER_INDEX = 0,
                VERTICAL_BLUR_BUFFER_INDEX = 1
            };

            VulkanShader m_BloomShader;

            VulkanDescriptorSetLayout m_DescriptorSetLayout;
            VulkanDescriptorSet m_DescriptorSets[2];

            VulkanTexture* m_ColorAttachments[BLUR_BUFFERS_COUNT];
            VulkanRenderPass m_Renderpasses[BLUR_BUFFERS_COUNT];

            std::vector<VulkanFramebuffer> m_Framebuffers[BLUR_BUFFERS_COUNT];
            
            VkPipelineLayout m_PipelineLayouts[BLUR_BUFFERS_COUNT];
            VulkanGraphicsPipeline m_Pipelines[BLUR_BUFFERS_COUNT];

            VulkanStaticMeshData m_QuadMesh;

            uint32_t m_FinalResultBufferIndex = 0;
        };
    }
}
