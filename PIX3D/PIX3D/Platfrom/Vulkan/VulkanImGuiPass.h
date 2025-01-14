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
            static void Render(VkCommandBuffer commandBuffer, uint32_t imageIndex);
            static void OnResize(uint32_t width, uint32_t height);

        private:
            static void CreateDescriptorPool();
            static void CreateRenderPass();

        private:
            inline static VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
            inline static VkRenderPass m_RenderPass = VK_NULL_HANDLE;
            inline static std::vector<VkFramebuffer> m_Framebuffers;
        };
	}
}
