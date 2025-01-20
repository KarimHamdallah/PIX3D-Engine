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

        // Import buttons
        if (ImGui::Button("Import Mesh", ImVec2(90, 20)))
        {
            auto* platform = Engine::GetPlatformLayer();
            std::filesystem::path filepath = platform->OpenDialogue(FileDialougeFilter::GLTF);
            if (!filepath.empty())
            {
                AssetManager::Get().LoadStaticMesh(filepath.string(), 1.0f);
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Import Texture", ImVec2(120, 20)))
        {
            auto* platform = Engine::GetPlatformLayer();
            std::filesystem::path filepath = platform->OpenDialogue(FileDialougeFilter::PNG);
            if (!filepath.empty())
            {
                AssetManager::Get().LoadTexture(filepath.string(), true, true);
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
        float maxSize = 100.0f;  // Maximum size for thumbnails
        float padding = 8.0f;
        float cellSize = maxSize + padding * 2;

        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = (int)(panelWidth / cellSize);
        if (columnCount < 1) columnCount = 1;

        ImGui::Columns(columnCount, 0, false);

        auto& assetManager = AssetManager::Get();
        float startY = ImGui::GetCursorPosY();  // Store starting Y position

        // Render Static Meshes
        for (const auto& [uuid, mesh] : assetManager.m_StaticMeshes)
        {
            const std::string& name = mesh->GetPath().filename().string();

            ImGui::PushID((uint64_t)uuid);
            ImGui::BeginGroup();

            // Calculate centered position
            float columnWidth = ImGui::GetColumnWidth();
            float thumbnailWidth = maxSize;
            float thumbnailHeight = maxSize;
            float centerX = ImGui::GetCursorPosX() + (columnWidth - thumbnailWidth) * 0.5f;

            // Set position explicitly
            ImGui::SetCursorPos(ImVec2(centerX, startY));

            bool isSelected = m_SelectedAsset == uuid;
            if (isSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
            }

            if (ImGui::ImageButton("##thumbnail",
                (ImTextureID)m_DefaultStaticMeshPreview->GetImGuiDescriptorSet(),
                ImVec2(thumbnailWidth, thumbnailHeight)))
            {
                m_SelectedAsset = uuid;
            }

            if (isSelected)
            {
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("ASSET_STATIC_MESH", &uuid, sizeof(UUID));
                ImGui::Image((ImTextureID)m_DefaultStaticMeshPreview->GetImGuiDescriptorSet(),
                    ImVec2(50, 50));
                ImGui::SameLine();
                ImGui::Text("%s", name.c_str());
                ImGui::EndDragDropSource();
            }

            // Center name under image
            ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
            ImGui::SetCursorPosX(centerX + (thumbnailWidth - textSize.x) * 0.5f);
            ImGui::TextUnformatted(name.c_str());

            ImGui::EndGroup();
            ImGui::PopID();
            ImGui::NextColumn();
        }

        // Reset cursor Y for textures
        ImGui::SetCursorPosY(startY);

        // Render Textures
        for (const auto& [uuid, texture] : assetManager.m_Textures)
        {
            const std::string& name = texture->GetPath().filename().string();

            ImGui::PushID((uint64_t)uuid);
            ImGui::BeginGroup();

            // Calculate aspect ratio and size
            float width = (float)texture->GetWidth();
            float height = (float)texture->GetHeight();
            float aspect = width / height;

            float thumbnailWidth, thumbnailHeight;
            if (width > height)
            {
                thumbnailWidth = maxSize;
                thumbnailHeight = maxSize / aspect;
            }
            else
            {
                thumbnailHeight = maxSize;
                thumbnailWidth = maxSize * aspect;
            }

            // Center in column
            float columnWidth = ImGui::GetColumnWidth();
            float centerX = ImGui::GetCursorPosX() + (columnWidth - thumbnailWidth) * 0.5f;

            // Set position explicitly
            ImGui::SetCursorPos(ImVec2(centerX, startY));

            bool isSelected = m_SelectedAsset == uuid;
            if (isSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
            }

            if (ImGui::ImageButton("##thumbnail",
                (ImTextureID)texture->GetImGuiDescriptorSet(),
                ImVec2(thumbnailWidth, thumbnailHeight)))
            {
                m_SelectedAsset = uuid;
            }

            if (isSelected)
            {
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("ASSET_TEXTURE", &uuid, sizeof(UUID));

                float previewSize = 50.0f;
                float previewWidth, previewHeight;
                if (width > height)
                {
                    previewWidth = previewSize;
                    previewHeight = previewSize / aspect;
                }
                else
                {
                    previewHeight = previewSize;
                    previewWidth = previewSize * aspect;
                }

                ImGui::Image((ImTextureID)texture->GetImGuiDescriptorSet(),
                    ImVec2(previewWidth, previewHeight));
                ImGui::SameLine();
                ImGui::Text("%s", name.c_str());
                ImGui::EndDragDropSource();
            }

            // Center name under image
            ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
            ImGui::SetCursorPosX(centerX + (thumbnailWidth - textSize.x) * 0.5f);
            ImGui::TextUnformatted(name.c_str());

            ImGui::EndGroup();
            ImGui::PopID();
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
    }

    void AssetWidget::RenderAssetContextMenu()
    {
        /*
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
        */
    }

    void AssetWidget::PostFrameProcesses()
    {
    }
}
