#pragma once
#include <Platfrom/Vulkan/VulkanGraphicsPipeline.h>
#include <Platfrom/Vulkan/VulkanShader.h>
#include <Platfrom/Vulkan/VulkanIndexBuffer.h>
#include <Platfrom/Vulkan/VulkanDescriptorSetLayout.h>
#include <Platfrom/Vulkan/VulkanDescriptorSet.h>
#include <Platfrom/Vulkan/VulkanVertexInputLayout.h>
#include <Platfrom/Vulkan/VulkanRenderpass.h>
#include <Platfrom/Vulkan/VulkanFramebuffer.h>
#include <Graphics/VulkanStaticMesh.h>
#include <Graphics/Camera3D.h>

namespace PIX3D
{
	namespace VK
	{
		class VulkanMainRenderpass
		{
		public:
			struct _CameraUniformBuffer
			{
				glm::mat4 view;
				glm::mat4 proj;
			};

		public:
			VulkanMainRenderpass() = default;
			~VulkanMainRenderpass() {}

			void Init(uint32_t width, uint32_t height);
			void BeginRender(Camera3D& cam, uint32_t image_index);
			void EndRender(uint32_t image_index);
			void RenderMesh(VulkanStaticMesh& mesh, uint32_t image_index);

		public:
			VK::VulkanShader m_Shader;

			std::vector<VkCommandBuffer> m_CommandBuffers;

			VK::VulkanGraphicsPipeline m_GraphicsPipeline;
			VkPipelineLayout m_PipelineLayout = nullptr;

			VK::VulkanVertexBuffer m_VertexBuffer;
			VK::VulkanVertexInputLayout m_VertexInputLayout;
			VK::VulkanIndexBuffer m_IndexBuffer;

			VK::VulkanTexture* m_ColorAttachmentTexture;
			VK::VulkanTexture* m_BloomBrightnessAttachmentTexture;

			VK::VulkanDescriptorSetLayout m_CameraDescriptorSetLayout;
			VK::VulkanRenderPass m_Renderpass;
			VK::VulkanFramebuffer m_Framebuffer;

			std::vector<VK::VulkanDescriptorSet> m_CameraDescriptorSets;
			std::vector<VK::VulkanUniformBuffer> m_CameraUniformBuffers;

			enum
			{
				BLOOM_DOWN_SAMPLES = 6
			};
		};
	}
}
