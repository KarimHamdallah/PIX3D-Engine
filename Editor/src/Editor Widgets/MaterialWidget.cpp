#include "MaterialWidget.h"
#include <Asset/AssetManager.h>
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
            ImGui::PushID(tag->m_UUID);
            ImGui::Text("Materials for: %s", tag->m_Tag.c_str());
            ImGui::Separator();
        }

        auto& m_Mesh = *PIX3D::AssetManager::Get().GetStaticMesh(meshComponent->m_AssetID);

        // Display materials for each submesh
        for (size_t i = 0; i < m_Mesh.m_SubMeshes.size(); i++)
        {
            auto& subMesh = m_Mesh.m_SubMeshes[i];
            if (subMesh.MaterialIndex < 0)
                continue;

            ImGui::PushID(static_cast<int>(i));
            auto& material = m_Mesh.m_Materials[subMesh.MaterialIndex];
            DrawMaterialUI(meshComponent, material, i);
            ImGui::PopID();
        }

        if (auto* tag = m_Scene->m_Registry.try_get<PIX3D::TagComponent>(m_HierarchyWidget->GetSelectedEntity()))
        {
            ImGui::PopID();
        }
    }

    ImGui::End();
}

void MaterialWidget::DrawMaterialUI(StaticMeshComponent* meshComponent, PIX3D::VulkanBaseColorMaterial& material, size_t submeshIndex)
{
    auto& m_Mesh = *PIX3D::AssetManager::Get().GetStaticMesh(meshComponent->m_AssetID);

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

        materialModified |= ImGui::Checkbox("Use Albedo Map", &material.UseAlbedoTexture);
        if (material.UseAlbedoTexture)
        {
            ImGui::SameLine();
            if (ImGui::Button("Load##Albedo"))
            {
                PendingTextureLoad load;
                load.submeshIndex = submeshIndex;
                load.materialIndex = m_Mesh.m_SubMeshes[submeshIndex].MaterialIndex;
                load.type = PendingTextureLoad::TextureType::Albedo;
                m_PendingTextureLoads.push_back(load);
            }

            ImGui::Image((ImTextureID)material.AlbedoTexture->GetImGuiDescriptorSet(),
                ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1));
        }
        ImGui::TreePop();
    }

    // Normal Map Section
    if (ImGui::TreeNodeEx("Normal Map", ImGuiTreeNodeFlags_DefaultOpen))
    {
        materialModified |= ImGui::Checkbox("Use Normal Map", &material.UseNormalTexture);
        if (material.UseNormalTexture)
        {
            ImGui::SameLine();
            if (ImGui::Button("Load##Normal"))
            {
                PendingTextureLoad load;
                load.submeshIndex = submeshIndex;
                load.materialIndex = m_Mesh.m_SubMeshes[submeshIndex].MaterialIndex;
                load.type = PendingTextureLoad::TextureType::Normal;
                m_PendingTextureLoads.push_back(load);
            }

            ImGui::Image((ImTextureID)material.NormalTexture->GetImGuiDescriptorSet(),
                ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1));
        }
        ImGui::TreePop();
    }

    // Metallic/Roughness Section
    if (ImGui::TreeNodeEx("Metallic/Roughness", ImGuiTreeNodeFlags_DefaultOpen))
    {
        materialModified |= ImGui::DragFloat("Metallic", &material.Metalic, 0.01f, 0.0f, 1.0f);
        materialModified |= ImGui::DragFloat("Roughness", &material.Roughness, 0.01f, 0.0f, 1.0f);

        materialModified |= ImGui::Checkbox("Use Metal-Rough Map", &material.UseMetallicRoughnessTexture);
        if (material.UseMetallicRoughnessTexture)
        {
            ImGui::SameLine();
            if (ImGui::Button("Load##MetalRough"))
            {
                PendingTextureLoad load;
                load.submeshIndex = submeshIndex;
                load.materialIndex = m_Mesh.m_SubMeshes[submeshIndex].MaterialIndex;
                load.type = PendingTextureLoad::TextureType::MetallicRoughness;
                m_PendingTextureLoads.push_back(load);
            }

            ImGui::Image((ImTextureID)material.MetalRoughnessTexture->GetImGuiDescriptorSet(),
                ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1));
        }
        ImGui::TreePop();
    }

    // AO Section
    if (ImGui::TreeNodeEx("Ambient Occlusion", ImGuiTreeNodeFlags_DefaultOpen))
    {
        materialModified |= ImGui::DragFloat("AO", &material.Ao, 0.01f, 0.0f, 1.0f);

        materialModified |= ImGui::Checkbox("Use AO Map", &material.useAoTexture);
        if (material.useAoTexture)
        {
            ImGui::SameLine();
            if (ImGui::Button("Load##AO"))
            {
                PendingTextureLoad load;
                load.submeshIndex = submeshIndex;
                load.materialIndex = m_Mesh.m_SubMeshes[submeshIndex].MaterialIndex;
                load.type = PendingTextureLoad::TextureType::AO;
                m_PendingTextureLoads.push_back(load);
            }

            ImGui::Image((ImTextureID)material.AoTexture->GetImGuiDescriptorSet(),
                ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1));
        }
        ImGui::TreePop();
    }

    // Emissive Section
    if (ImGui::TreeNodeEx("Emissive", ImGuiTreeNodeFlags_DefaultOpen))
    {
        materialModified |= ImGui::ColorEdit4("Color", &material.EmissiveColor.x);

        materialModified |= ImGui::Checkbox("Use Emissive Map", &material.UseEmissiveTexture);
        if (material.UseEmissiveTexture)
        {
            ImGui::SameLine();
            if (ImGui::Button("Load##Emissive"))
            {
                PendingTextureLoad load;
                load.submeshIndex = submeshIndex;
                load.materialIndex = m_Mesh.m_SubMeshes[submeshIndex].MaterialIndex;
                load.type = PendingTextureLoad::TextureType::Emissive;
                m_PendingTextureLoads.push_back(load);
            }

            ImGui::Image((ImTextureID)material.EmissiveTexture->GetImGuiDescriptorSet(),
                ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1));
        }
        ImGui::TreePop();
    }

    if (materialModified)
    {

        if (auto* meshComp = m_Scene->m_Registry.try_get<PIX3D::StaticMeshComponent>(m_HierarchyWidget->GetSelectedEntity()))
        {
            auto& m_Mesh = *PIX3D::AssetManager::Get().GetStaticMesh(meshComponent->m_AssetID);
            m_Mesh.FillMaterialBuffer();
        }
    }
}

void MaterialWidget::PostFrameProcesses()
{
    if (m_PendingTextureLoads.empty())
        return;

    auto selectedEntity = m_HierarchyWidget->GetSelectedEntity();
    if (selectedEntity == entt::null)
        return;

    auto* meshComponent = m_Scene->m_Registry.try_get<PIX3D::StaticMeshComponent>(selectedEntity);
    if (!meshComponent)
        return;

    auto* platform = PIX3D::Engine::GetPlatformLayer();

    for (const auto& load : m_PendingTextureLoads)
    {
        // Validate indices

        auto& m_Mesh = *PIX3D::AssetManager::Get().GetStaticMesh(meshComponent->m_AssetID);

        if (load.submeshIndex >= m_Mesh.m_SubMeshes.size())
            continue;

        auto& subMesh = m_Mesh.m_SubMeshes[load.submeshIndex];
        if (subMesh.MaterialIndex < 0 || subMesh.MaterialIndex != load.materialIndex)
            continue;

        auto& material = m_Mesh.m_Materials[subMesh.MaterialIndex];

        // Open file dialog
        std::filesystem::path filepath = platform->OpenDialogue(PIX3D::FileDialougeFilter::PNG);
        if (filepath.empty())
            continue;

        // Load the appropriate texture based on type
        switch (load.type)
        {
        case PendingTextureLoad::TextureType::Albedo:
        {
            material.AlbedoTexture->Destroy();
            material.AlbedoTexture = new VK::VulkanTexture();
            material.AlbedoTexture->LoadFromFile(filepath.string(), true, true);
            break;
        }
        case PendingTextureLoad::TextureType::Normal:
        {
            material.NormalTexture->Destroy();
            material.NormalTexture = new VK::VulkanTexture();
            material.NormalTexture->LoadFromFile(filepath.string());
            break;
        }
        case PendingTextureLoad::TextureType::MetallicRoughness:
        {
            material.MetalRoughnessTexture->Destroy();
            material.MetalRoughnessTexture = new VK::VulkanTexture();
            material.MetalRoughnessTexture->LoadFromFile(filepath.string());
            break;
        }
        case PendingTextureLoad::TextureType::AO:
        {
            material.AoTexture->Destroy();
            material.AoTexture = new VK::VulkanTexture();
            material.AoTexture->LoadFromFile(filepath.string());
            break;
        }
        case PendingTextureLoad::TextureType::Emissive:
        {
            material.EmissiveTexture->Destroy();
            material.EmissiveTexture = new VK::VulkanTexture();
            material.EmissiveTexture->LoadFromFile(filepath.string());
            break;
        }
        }

        m_Mesh.FillMaterialBuffer();
    }

    m_PendingTextureLoads.clear();
}
