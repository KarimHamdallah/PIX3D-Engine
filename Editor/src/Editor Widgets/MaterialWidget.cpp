#include "MaterialWidget.h"
#include <imgui.h>

void MaterialWidget::OnRender()
{
    bool nulldata = false;

    if (m_HierarchyWidget->GetSelectedEntity() == entt::null || !m_Scene)
        nulldata = true;

    auto* meshComponent = m_Scene->m_Registry.try_get<PIX3D::StaticMeshComponent>(m_HierarchyWidget->GetSelectedEntity());
    if (!meshComponent)
        nulldata = true;

    ImGui::Begin("Materials");

    if (!nulldata)
    {
        // Header with mesh info
        if (auto* tag = m_Scene->m_Registry.try_get<PIX3D::TagComponent>(m_HierarchyWidget->GetSelectedEntity()))
        {
            ImGui::Text("Materials for: %s", tag->m_Tag.c_str());
            ImGui::Separator();
        }

        // Display materials for each submesh
        for (size_t i = 0; i < meshComponent->m_Mesh.m_SubMeshes.size(); i++)
        {
            auto& subMesh = meshComponent->m_Mesh.m_SubMeshes[i];
            if (subMesh.MaterialIndex < 0)
                continue;

            auto& material = meshComponent->m_Mesh.m_Materials[subMesh.MaterialIndex];
            DrawMaterialUI(material);
        }
    }

    ImGui::End();
}

void MaterialWidget::DrawMaterialUI(PIX3D::BaseColorMaterial& material)
{
    if (!ImGui::CollapsingHeader(material.Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        return;

    bool materialModified = false;

    // IBL Data
    if (ImGui::TreeNodeEx("IBL Data", ImGuiTreeNodeFlags_DefaultOpen))
    {
        materialModified |= ImGui::Checkbox("Use IBL", &material.UseIBL);
        ImGui::TreePop();
    }

    ImGui::Separator();

    // Base Color Section
    if (ImGui::TreeNodeEx("Base Color", ImGuiTreeNodeFlags_DefaultOpen))
    {
        materialModified |= ImGui::ColorEdit4("Color", &material.BaseColor.x);

        ImGui::Checkbox("Use Albedo Map", &material.UseAlbedoTexture);
        if (material.UseAlbedoTexture)
        {
            ImGui::SameLine();
            if (ImGui::Button("Load##Albedo"))
            {
                auto* platform = PIX3D::Engine::GetPlatformLayer();
                std::filesystem::path filepath = platform->OpenDialogue(PIX3D::FileDialougeFilter::PNG);
                if (!filepath.empty())
                {
                    material.AlbedoTexture.LoadFromFile(filepath.string(), true);
                    materialModified = true;
                }
            }

            ImGui::Image((ImTextureID)material.AlbedoTexture.GetHandle(), ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1));
        }
        ImGui::TreePop();
    }

    ImGui::Separator();

    // Normal Map Section
    if (ImGui::TreeNodeEx("Normal Map", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Use Normal Map", &material.UseNormalTexture);
        if (material.UseNormalTexture)
        {
            ImGui::SameLine();
            if (ImGui::Button("Load##Normal"))
            {
                auto* platform = PIX3D::Engine::GetPlatformLayer();
                std::filesystem::path filepath = platform->OpenDialogue(PIX3D::FileDialougeFilter::PNG);
                if (!filepath.empty())
                {
                    material.NormalTexture.LoadFromFile(filepath.string());
                    materialModified = true;
                }
            }

            ImGui::Image((ImTextureID)material.NormalTexture.GetHandle(),
                ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1));
        }
        ImGui::TreePop();
    }

    ImGui::Separator();

    // Metallic/Roughness Section
    if (ImGui::TreeNodeEx("Metallic/Roughness", ImGuiTreeNodeFlags_DefaultOpen))
    {
        materialModified |= ImGui::DragFloat("Metallic", &material.Metalic, 0.01f, 0.0f, 1.0f);
        materialModified |= ImGui::DragFloat("Roughness", &material.Roughness, 0.01f, 0.0f, 1.0f);

        ImGui::Checkbox("Use Metal-Rough Map", &material.UseMetallicRoughnessTexture);
        if (material.UseMetallicRoughnessTexture)
        {
            ImGui::SameLine();
            if (ImGui::Button("Load##MetalRough"))
            {
                auto* platform = PIX3D::Engine::GetPlatformLayer();
                std::filesystem::path filepath = platform->OpenDialogue(PIX3D::FileDialougeFilter::PNG);
                if (!filepath.empty())
                {
                    material.MetalRoughnessTexture.LoadFromFile(filepath.string());
                    materialModified = true;
                }
            }

            ImGui::Image((ImTextureID)material.MetalRoughnessTexture.GetHandle(),
                ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1));
        }
        ImGui::TreePop();
    }

    ImGui::Separator();

    // Ambient Occlusion Section
    if (ImGui::TreeNodeEx("Ambient Occlusion", ImGuiTreeNodeFlags_DefaultOpen))
    {
        materialModified |= ImGui::DragFloat("AO", &material.Ao, 0.01f, 0.0f, 1.0f);

        ImGui::Checkbox("Use AO Map", &material.useAoTexture);
        if (material.useAoTexture)
        {
            ImGui::SameLine();
            if (ImGui::Button("Load##AO"))
            {
                auto* platform = PIX3D::Engine::GetPlatformLayer();
                std::filesystem::path filepath = platform->OpenDialogue(PIX3D::FileDialougeFilter::PNG);
                if (!filepath.empty())
                {
                    material.AoTexture.LoadFromFile(filepath.string());
                    materialModified = true;
                }
            }

            ImGui::Image((ImTextureID)material.AoTexture.GetHandle(),
                ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1));
        }
        ImGui::TreePop();
    }

    ImGui::Separator();

    // Emissive Section
    if (ImGui::TreeNodeEx("Emissive", ImGuiTreeNodeFlags_DefaultOpen))
    {
        materialModified |= ImGui::ColorEdit4("Color", &material.EmissiveColor.x);

        ImGui::Checkbox("Use Emissive Map", &material.UseEmissiveTexture);
        if (material.UseEmissiveTexture)
        {
            ImGui::SameLine();
            if (ImGui::Button("Load##Emissive"))
            {
                auto* platform = PIX3D::Engine::GetPlatformLayer();
                std::filesystem::path filepath = platform->OpenDialogue(PIX3D::FileDialougeFilter::PNG);
                if (!filepath.empty())
                {
                    material.EmissiveTexture.LoadFromFile(filepath.string());
                    materialModified = true;
                }
            }

            ImGui::Image((ImTextureID)material.EmissiveTexture.GetHandle(),
                ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1));
        }
        ImGui::TreePop();
    }

    ImGui::Separator();

    /*
    // Update material buffer if any changes were made
    if (materialModified)
    {
        if (auto* meshComp = m_Scene->m_Registry.try_get<PIX3D::StaticMeshComponent>(m_HierarchyWidget->GetSelectedEntity()))
        {
            meshComp->m_Mesh.FillMaterialBuffer();
        }
    }
    */
}
