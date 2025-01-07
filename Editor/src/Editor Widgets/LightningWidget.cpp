#include "LightningWidget.h"
#include <imgui.h>

namespace
{
	void RenderTransformComponent(PIX3D::Transform& transform, const std::string& label)
	{
		// Use a unique label or ID scope
		ImGui::PushID(label.c_str());

		ImGui::Text("%s", label.c_str());
		ImGui::Separator();

		// Position
		ImGui::Text("Position");
		ImGui::DragFloat3(("Position##" + label).c_str(), &transform.Position.x, 0.1f, -1000.0f, 1000.0f, "%.3f");

		// Rotation
		ImGui::Text("Rotation");
		ImGui::DragFloat3(("Rotation##" + label).c_str(), &transform.Rotation.x, 0.1f, -360.0f, 360.0f, "%.3f");

		// Scale
		ImGui::Text("Scale");
		ImGui::DragFloat3(("Scale##" + label).c_str(), &transform.Scale.x, 0.1f, 0.0f, 1000.0f, "%.3f");

		ImGui::Separator();

		// Pop the ID scope
		ImGui::PopID();
	}
}

void LightningWidget::OnRender()
{
	ImGui::Begin("Lightning");

	ImGui::CollapsingHeader("Environment map");

	auto WidgetSize = ImGui::GetContentRegionAvail();

	ImGui::Checkbox("Use Skybox", &m_Scene->m_UseSkybox);
	ImGui::ColorEdit4("Background Color", &m_Scene->m_BackgroundColor.x);

	ImGui::SliderInt("Environment Map Size", &m_Scene->m_EnvironmentMapSize, 0, 4000);

	if (ImGui::Button("Load Environment Map", { WidgetSize.x, 20.0f }))
	{
		// Load .Hdr
		auto* platform = PIX3D::Engine::GetPlatformLayer();
		std::filesystem::path savepath = platform->OpenDialogue(PIX3D::FileDialougeFilter::HDR);
		if (!savepath.empty() && savepath.extension().string() == ".hdr")
		{
			std::cout << "Reload EnvMap\n";
			
			// Destroy Current Environment Map Data
			m_Scene->m_Cubemap.Destroy();
			m_Scene->m_IBLMaps.Destroy();
			
			m_Scene->m_Cubemap.LoadHdrToCubemapGPU(savepath, m_Scene->m_EnvironmentMapSize);
			m_Scene->m_IBLMaps = PIX3D::GL::IBLCubemapsGenerator::GenerateIBLMaps(m_Scene->m_Cubemap.GetHandle(), 32, 128);
		}
	}

	ImGui::Image((ImTextureID)m_Scene->m_Cubemap.GetHdrTextureHandle(), { WidgetSize.x, 200.0f }, { 0, 1 }, { 1, 0 });

	RenderTransformComponent(m_Scene->m_CubemapTransform, "Environment Map Transform");

	ImGui::CollapsingHeader("Post-processing");

	// Bloom
	ImGui::Text("Bloom (Gaussian)");
	ImGui::Checkbox("Enabled", &GL::GLRenderer::s_PostProcessingpass.m_BloomEnabled);
	ImGui::SliderFloat("Intensity", &GL::GLRenderer::s_PostProcessingpass.m_BloomIntensity, 0.0, 5.0);
	ImGui::SliderFloat("Threshold", &GL::GLRenderer::s_BloomThreshold, 0.01, 5.0);
	ImGui::SliderInt("Blur Iterations", &GL::GLRenderer::s_Bloompass.m_BloomIterations, 2, 20);
	ImGui::Text("Direction: "); ImGui::SameLine();
	ImGui::RadioButton("Both", &GL::GLRenderer::s_Bloompass.m_BloomDirection, 0); ImGui::SameLine();
	ImGui::RadioButton("Horizontal", &GL::GLRenderer::s_Bloompass.m_BloomDirection, 1); ImGui::SameLine();
	ImGui::RadioButton("Vertical", &GL::GLRenderer::s_Bloompass.m_BloomDirection, 2);

	// Composite
	ImGui::Text("Post");
	ImGui::Checkbox("HDR Tone Mapping (Reinhard)", &GL::GLRenderer::s_PostProcessingpass.m_TonemappingEnabled);
	ImGui::SliderFloat("Gamma Correction", &GL::GLRenderer::s_PostProcessingpass.m_GammaCorrectionFactor, 1.0, 3.0);
	ImGui::End();
}
