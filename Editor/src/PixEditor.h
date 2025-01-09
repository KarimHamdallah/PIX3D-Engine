#pragma once

#include <PIX3D.h>
#include <Platfrom/Vulkan/VulkanGraphicsPipeline.h>
#include <Platfrom/Vulkan/VulkanShader.h>
#include <Platfrom/Vulkan/VulkanIndexBuffer.h>
#include <Platfrom/Vulkan/VulkanDescriptorSetLayout.h>
#include <Platfrom/Vulkan/VulkanDescriptorSet.h>
#include <Platfrom/Vulkan/VulkanVertexInputLayout.h>
#include <Platfrom/Vulkan/VulkanRenderpass.h>
#include <Platfrom/Vulkan/VulkanFramebuffer.h>
#include <Platfrom/Vulkan/VulkanFullScreenQuadRenderpass.h>

using namespace PIX3D;

class PixEditor : public PIX3D::Application
{
public:
	struct _CameraUniformBuffer
	{
		glm::mat4 view;
		glm::mat4 proj;
	};


public:
	virtual void OnStart() override;
	virtual void OnUpdate(float dt) override;
	virtual void OnDestroy() override;
	virtual void OnResize(uint32_t width, uint32_t height) override;
	virtual void OnKeyPressed(uint32_t key) override;

private:
	int m_NumImages = 0;
	std::vector<VkCommandBuffer> m_CommandBuffers;

	VK::VulkanShaderStorageBuffer m_ShaderStorageBuffer;

	VK::VulkanShader m_TriangleShader;
	VK::VulkanGraphicsPipeline m_GraphicsPipeline;
	VkPipelineLayout m_PipelineLayout = nullptr;

	VK::VulkanVertexBuffer m_VertexBuffer;
	VK::VulkanVertexInputLayout m_VertexInputLayout;
	VK::VulkanIndexBuffer m_IndexBuffer;
	
	VK::VulkanTexture* m_ColorAttachmentTexture;
	VK::VulkanTexture* m_BloomBrightnessAttachmentTexture;
	
	VK::VulkanDescriptorSetLayout m_CameraDescriptorSetLayout;
	VK::VulkanRenderPass m_Renderpass;
	std::vector<VK::VulkanFramebuffer> m_Framebuffers;
	
	std::vector<VK::VulkanDescriptorSet> m_CameraDescriptorSets;
	std::vector<VK::VulkanUniformBuffer> m_CameraUniformBuffers;

	PIX3D::Camera3D Cam;

	PIX3D::VulkanStaticMesh m_Mesh;

	PIX3D::VK::VulkanFullScreenQuadRenderpass m_FullScreenQuadRenderpass;
};
