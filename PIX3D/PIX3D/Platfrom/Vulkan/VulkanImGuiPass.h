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

            void Init(void* window_handle, uint32_t width, uint32_t height);
            void Destroy();
            void BeginFrame();
            void EndFrame();
            void Render(VkCommandBuffer commandBuffer, uint32_t imageIndex);
            void OnResize(uint32_t width, uint32_t height);

        private:
            void CreateDescriptorPool();
            void CreateRenderPass();

        private:
            VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
            VkRenderPass m_RenderPass = VK_NULL_HANDLE;
            std::vector<VkFramebuffer> m_Framebuffers;
        };
	}
}
