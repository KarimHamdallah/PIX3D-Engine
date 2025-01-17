#pragma once
#include <vulkan/vulkan.h>
#include <vector>
namespace PIX3D
{
    namespace VK
    {
        class VulkanImGuiPass
        {
        public:
            VulkanImGuiPass() {}
            ~VulkanImGuiPass() {}

            static void Init(uint32_t width, uint32_t height);
            static void Destroy();
            static void BeginFrame();
            static void EndFrame();
            static void Render(VkCommandBuffer commandBuffer, uint32_t imageIndex, bool useLoadRenderPass = true);
            static void OnResize(uint32_t width, uint32_t height);
            static void StartDockSpace();
            static void EndDockSpace();

            static void BeginRecordCommandbuffer(uint32_t ImageIndex);
            static void EndRecordCommandbufferAndSubmit(uint32_t ImageIndex, bool useLoadRenderPass = true);
        private:
            static void CreateDescriptorPool();
            static void CreateRenderPasses();
        private:
            inline static VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
            inline static VkRenderPass m_LoadRenderPass = VK_NULL_HANDLE;    // For overlay rendering
            inline static VkRenderPass m_ClearRenderPass = VK_NULL_HANDLE;   // For standalone UI
            inline static std::vector<VkFramebuffer> m_LoadFramebuffers;     // Framebuffers for overlay
            inline static std::vector<VkFramebuffer> m_ClearFramebuffers;    // Framebuffers for standalone
            
        public:
            inline static std::vector<VkCommandBuffer> m_CommandBuffers;
        };
    }
}
