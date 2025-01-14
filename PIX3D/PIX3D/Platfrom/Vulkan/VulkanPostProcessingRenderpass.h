#pragma once

#include <Platfrom/Vulkan/VulkanGraphicsPipeline.h>
#include <Platfrom/Vulkan/VulkanShader.h>
#include <Platfrom/Vulkan/VulkanIndexBuffer.h>
#include <Platfrom/Vulkan/VulkanDescriptorSetLayout.h>
#include <Platfrom/Vulkan/VulkanDescriptorSet.h>
#include <Platfrom/Vulkan/VulkanRenderpass.h>
#include <Platfrom/Vulkan/VulkanFramebuffer.h>
#include <Platfrom/Vulkan/VulkanStaticMeshFactory.h>


namespace PIX3D
{
	namespace VK
	{
		class VulkanPostProcessingRenderpass
		{
		public:
			VulkanPostProcessingRenderpass() = default;
			~VulkanPostProcessingRenderpass() {}

			void Init(uint32_t width, uint32_t height, VulkanTexture* color_attachment, VulkanTexture* bloom_attachment);
			void Resize(uint32_t width, uint32_t height, VulkanTexture* color_attachment, VulkanTexture* bloom_attachment);
			void Destroy();

			void RecordCommandBuffer(VkCommandBuffer commandbuffer, uint32_t ImageIndex);

		private:

			uint32_t m_Width, m_Height = 0;

			VulkanShader m_Shader;

			VulkanDescriptorSetLayout m_DescriptorSetLayout;
			VulkanDescriptorSet m_DescriptorSet;

			VulkanRenderPass m_Renderpass;
			std::vector<VulkanFramebuffer> m_Framebuffers;

			VulkanGraphicsPipeline m_GraphicsPipeline;
			VkPipelineLayout m_PipelineLayout = nullptr;

			VulkanStaticMeshData m_QuadMesh;
		};
	}
}
