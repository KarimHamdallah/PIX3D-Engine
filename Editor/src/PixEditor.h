#pragma once

#include <PIX3D.h>
#include <Platfrom/Vulkan/VulkanMainRenderpass.h>
#include <Platfrom/Vulkan/VulkanPostProcessingRenderpass.h>
#include <Platfrom/Vulkan/VulkanBloomPass.h>
#include <Platfrom/Vulkan/VulkanComputePipeline.h>

using namespace PIX3D;

class PixEditor : public PIX3D::Application
{
public:

public:
	virtual void OnStart() override;
	virtual void OnUpdate(float dt) override;
	virtual void OnDestroy() override;
	virtual void OnResize(uint32_t width, uint32_t height) override;
	virtual void OnKeyPressed(uint32_t key) override;

private:
	int m_NumImages = 0;

	PIX3D::Camera3D Cam;

	PIX3D::VulkanStaticMesh m_Mesh;

	PIX3D::VK::VulkanMainRenderpass m_MainRenderpass;
	PIX3D::VK::VulkanPostProcessingRenderpass m_FullScreenQuadRenderpass;
	PIX3D::VK::VulkanBloomPass m_BloomPass;


	VK::VulkanShader m_ComputeShader;

	VK::VulkanShaderStorageBuffer m_ComputeStorageBuffer;
	VK::VulkanDescriptorSetLayout m_ComputeDescriptorSetLayout;
	VK::VulkanDescriptorSet m_ComputeDescriptorSet;

	VK::VulkanComputePipeline m_ComputePipeline;
};
