#include "FrostedGlassExample.h"

void FrostedGlassExample::OnStart()
{
	auto Specs = Engine::GetApplicationSpecs();

	m_Scene = new Scene("FrostedGlassScene");
	m_Scene->OnStart();

	auto* Mesh = AssetManager::Get().LoadStaticMesh("res/otako/source/model.fbx", 0.01f);

	TransformData MeshTransform;
	m_Scene->AddStaticMesh("Mesh", MeshTransform, Mesh->GetUUID());

	m_FrostedGlassMaskRenderpass = new FrostedGlassMaskRenderpass();
	m_FrostedGlassMaskRenderpass->Init(Specs.Width, Specs.Height);

	m_CubeMesh= new VulkanStaticMesh();
	m_CubeMesh->Load("res/Cube/Cube obj.obj");
	m_CubeTransform.m_Position.z = 0.5f;
	m_CubeTransform.m_Scale = { 0.5f, 0.5f, 0.05f };

	m_FrostedGlassPostProcessPass = new VK::FrostedGlassPostProcessPass();
	m_FrostedGlassPostProcessPass->Init(Specs.Width, Specs.Height);

	m_FrostedGlassBlurPass = new VK::FrostedGlassBlurPass();
	m_FrostedGlassBlurPass->Init(Specs.Width, Specs.Height);
}

void FrostedGlassExample::OnUpdate(float dt)
{


	m_Scene->OnUpdate(dt);
	
	auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
	
	// Render Opaque Objects
	m_Scene->OnRender(false);

	uint32_t ImageIndex = VK::VulkanSceneRenderer::s_ImageIndex;

	// Render Glass Objects I World Space
	m_FrostedGlassMaskRenderpass->Render(*m_CubeMesh, m_CubeTransform);
	
	// Mask Areas Behind Glass
	m_FrostedGlassPostProcessPass->Render(VK::VulkanSceneRenderer::s_MainRenderpass.ColorAttachmentTexture, m_FrostedGlassMaskRenderpass->GetOutputTexture());
	
	// Blur Masjed Texture
	m_FrostedGlassBlurPass->Process(m_FrostedGlassPostProcessPass->GetOutputTexture());

	// Full Screen Quad
	VK::VulkanSceneRenderer::s_PostProcessingRenderpass.RecordCommandBuffer(
		VK::VulkanSceneRenderer::s_MainRenderpass.ColorAttachmentTexture,
		VK::VulkanSceneRenderer::s_BloomPass.GetFinalBloomTexture(),
		m_FrostedGlassBlurPass->GetOutputTexture(),
		VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[ImageIndex],
		ImageIndex);

	VK::VulkanSceneRenderer::EndRecordCommandBuffer();

	Context->m_Queue.SubmitAsyncBuffers(
		{
			VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[ImageIndex]
		});
	Context->m_Queue.Present(VK::VulkanSceneRenderer::s_ImageIndex);
}

void FrostedGlassExample::OnDestroy()
{

}

void FrostedGlassExample::OnKeyPressed(uint32_t key)
{

}
