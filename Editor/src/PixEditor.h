#pragma once

#include <PIX3D.h>
#include <Platfrom/Vulkan/VulkanGraphicsPipeline.h>
#include <Platfrom/Vulkan/VulkanShader.h>
#include <Platfrom/Vulkan/VulkanShaderStorageBuffer.h>

using namespace PIX3D;

class PixEditor : public PIX3D::Application
{
public:
	virtual void OnStart() override;
	virtual void OnUpdate(float dt) override;
	virtual void OnDestroy() override;
	virtual void OnResize(uint32_t width, uint32_t height) override;
	virtual void OnKeyPressed(uint32_t key) override;

private:
	int m_NumImages = 0;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	VkRenderPass m_Renderpass;
	std::vector<VkFramebuffer> m_FrameBuffers;

	VK::VulkanShader m_TriangleShader;
	VK::VulkanGraphicsPipeline m_GraphicsPipeline;
	VK::VulkanVertexBuffer m_VertexBuffer;
	VK::VulkanShaderStorageBuffer m_ShaderStorageBuffer;
};
