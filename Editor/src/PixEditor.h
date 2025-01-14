#pragma once

#include <PIX3D.h>
#include <Platfrom/Vulkan/VulkanSceneRenderer.h>
#include <Platfrom/Vulkan/VulkanComputePipeline.h>
#include <Platfrom/Vulkan/VulkanImGuiPass.h>
#include <Graphics/Material.h>

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

	VK::VulkanShader m_ComputeShader;

	VK::VulkanShaderStorageBuffer m_ComputeStorageBuffer;
	VK::VulkanDescriptorSetLayout m_ComputeDescriptorSetLayout;
	VK::VulkanDescriptorSet m_ComputeDescriptorSet;

	VK::VulkanComputePipeline m_ComputePipeline;

	TransformComponent m_SpriteTransform;
	SpriteMaterial m_Material;
};
