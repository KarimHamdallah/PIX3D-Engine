#include "AssetWidget.h"
#include "Asset/AssetManager.h"
#include <imgui.h>
#include <Engine/Engine.hpp>

namespace PIX3D
{
    AssetWidget::AssetWidget()
    {
        m_DefaultStaticMeshPreview = new VK::VulkanTexture();
        m_DefaultStaticMeshPreview->LoadFromFile("res/icons/3d-file.png");
    }

    AssetWidget::~AssetWidget()
    {
        m_DefaultStaticMeshPreview->Destroy();
        delete m_DefaultStaticMeshPreview;
    }

    void AssetWidget::OnRender()
    {
        ImGui::Begin("Assets");

        // Import button
        if (ImGui::Button("Import Mesh"))
        {
            auto* platform = Engine::GetPlatformLayer();
            std::filesystem::path filepath = platform->OpenDialogue(FileDialougeFilter::GLTF);
            if (!filepath.empty())
            {
                AssetManager::Get().LoadStaticMesh(filepath.string(), 1.0f);
            }
        }

        ImGui::Separator();

        // Grid of assets
        RenderAssetGrid();

        // Context menu
        RenderAssetContextMenu();

        ImGui::End();
    }

    void AssetWidget::RenderAssetGrid()
    {
        float padding = 8.0f;
        float thumbnailSize = 100.0f;
        float cellSize = thumbnailSize + padding * 2;

        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = (int)(panelWidth / cellSize);
        if (columnCount < 1) columnCount = 1;

        ImGui::Columns(columnCount, 0, false);

        auto& assetManager = AssetManager::Get();
        for (const auto& [uuid, mesh] : assetManager.m_StaticMeshes)
        {
            const std::string& name = mesh->GetPath().filename().string();

            ImGui::PushID(uuid);
            ImGui::BeginGroup();

            // Center the thumbnail button
            float columnWidth = ImGui::GetColumnWidth();
            float centerX = ImGui::GetCursorPosX() + (columnWidth - thumbnailSize) * 0.5f;
            ImGui::SetCursorPosX(centerX);

            // Thumbnail button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.4f, 0.7f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.5f, 0.8f, 1.0f));

            if (ImGui::ImageButton("##thumbnail", (ImTextureID)m_DefaultStaticMeshPreview->GetImGuiDescriptorSet(), ImVec2(thumbnailSize, thumbnailSize)))
            {
                m_SelectedAsset = uuid;
            }
            ImGui::PopStyleColor(3);

            // Drag source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("ASSET_STATIC_MESH", &uuid, sizeof(UUID));
                ImGui::Text("%s", name.c_str());
                ImGui::EndDragDropSource();
            }

            // Center and display the name directly under the thumbnail
            ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
            ImGui::SetCursorPosX(centerX + (thumbnailSize - textSize.x) * 0.5f);
            ImGui::TextUnformatted(name.c_str());

            ImGui::EndGroup();
            ImGui::PopID();
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
    }

    void AssetWidget::RenderAssetContextMenu()
    {
        if (ImGui::BeginPopupContextWindow("Asset Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
            if (m_SelectedAsset != UUID())
            {
                if (ImGui::MenuItem("Delete"))
                {
                    AssetManager::Get().UnloadStaticMesh(m_SelectedAsset);
                    m_SelectedAsset = UUID();
                }
            }
            ImGui::EndPopup();
        }
    }

    void AssetWidget::HandleDragDrop()
    {

    }

    void AssetWidget::PostFrameProcesses()
    {
    }
}
