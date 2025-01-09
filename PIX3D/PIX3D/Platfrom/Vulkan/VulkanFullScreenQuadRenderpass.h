#pragma once

#include <Platfrom/Vulkan/VulkanGraphicsPipeline.h>
#include <Platfrom/Vulkan/VulkanShader.h>
#include <Platfrom/Vulkan/VulkanIndexBuffer.h>
#include <Platfrom/Vulkan/VulkanDescriptorSetLayout.h>
#include <Platfrom/Vulkan/VulkanDescriptorSet.h>
#include <Platfrom/Vulkan/VulkanVertexInputLayout.h>
#include <Platfrom/Vulkan/VulkanRenderpass.h>
#include <Platfrom/Vulkan/VulkanFramebuffer.h>


namespace PIX3D
{
	namespace VK
	{
		class VulkanFullScreenQuadRenderpass
		{
		public:
			VulkanFullScreenQuadRenderpass() = default;
			~VulkanFullScreenQuadRenderpass() {}

			void Init(uint32_t width, uint32_t height, VulkanTexture* ColorAttachment);
			void Destroy();

			void Render();

		private:

			VulkanShader m_Shader;

			VulkanVertexBuffer m_VertexBuffer;
			VulkanVertexInputLayout m_VertexInputLayout;
			VulkanIndexBuffer m_IndexBuffer;

			VulkanDescriptorSetLayout m_DescriptorSetLayout;
			VulkanDescriptorSet m_DescriptorSet;

			VulkanRenderPass m_Renderpass;
			std::vector<VulkanFramebuffer> m_Framebuffers;

			VulkanGraphicsPipeline m_GraphicsPipeline;

			std::vector<VkCommandBuffer> m_CommandBuffers;
		};
	}
}
