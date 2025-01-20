#include "InspectorWidget.h"
//#include <imGuIZMO.quat/imGuIZMOquat.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <Asset/AssetManager.h>

namespace
{
    static void DrawVec3Control(const std::string& lable, glm::vec3& value, float resetvalue = 0.0f, float Speed = 0.01f, float colwidth = 80.0f)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImFont* BoldFont = io.Fonts->Fonts[0];

        ImGui::PushID(lable.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, colwidth);
        ImGui::Text(lable.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0.0f, 0.0f });

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 ButtonSize = { lineHeight + 3.0f, lineHeight };

        ImGui::PushStyleColor(ImGuiCol_Button, { 0.7f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.9f, 0.4f, 0.4f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushFont(BoldFont);
        if (ImGui::Button("X", ButtonSize))
            value.x = resetvalue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &value.x, Speed);
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, { 0.15f, 0.8f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.4f, 0.9f, 0.4f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.1f, 0.8f, 0.15f, 1.0f });
        ImGui::PushFont(BoldFont);
        if (ImGui::Button("Y", ButtonSize))
            value.y = resetvalue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &value.y, Speed);
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, { 0.15f, 0.1f, 0.8f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.4f, 0.4f, 0.9f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.1f, 0.1f, 0.8f, 1.0f });
        ImGui::PushFont(BoldFont);
        if (ImGui::Button("Z", ButtonSize))
            value.z = resetvalue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &value.z, Speed);
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);
        ImGui::PopID();
    }

    /*
    vgm::Quat qRot = quat(1.f, 0.f, 0.f, 0.f);
    vgm::Vec3 PanDolly(0.f);
    static vec3 lightDirection = vec3(0.0f, -1.0f, 0.0f); // Initial direction pointing down
    */
}

void InspectorWidget::OnRender()
{
    ImGui::Begin("Inspector");

    auto selectedEntity = m_HierarchyWidget->GetSelectedEntity();
    if (selectedEntity != entt::null)
    {
        // Tag Component
        if (auto* tag = m_Scene->m_Registry.try_get<TagComponent>(selectedEntity))
        {
            std::string uuidDisplay = "UUID: " + std::to_string(tag->m_UUID);
            ImGui::TextDisabled("%s", uuidDisplay.c_str());

            char buffer[256];
            strcpy_s(buffer, sizeof(buffer), tag->m_Tag.c_str());
            if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
            {
                tag->m_Tag = std::string(buffer);
            }
        }

        ImGui::SameLine();

        // Add Component Button
        if (ImGui::Button("Add Component"))
        {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (ImGui::BeginPopup("AddComponentPopup"))
        {
            if (ImGui::MenuItem("Transform"))
            {
                if (!m_Scene->m_Registry.try_get<TransformComponent>(selectedEntity))
                    m_Scene->m_Registry.emplace<TransformComponent>(selectedEntity);
            }
            if (ImGui::MenuItem("Static Mesh"))
            {
                if (!m_Scene->m_Registry.try_get<StaticMeshComponent>(selectedEntity))
                    m_Scene->m_Registry.emplace<StaticMeshComponent>(selectedEntity);
            }
            if (ImGui::MenuItem("Sprite"))
            {
                if (!m_Scene->m_Registry.try_get<SpriteComponent>(selectedEntity))
                    m_Scene->m_Registry.emplace<SpriteComponent>(selectedEntity);
            }
            if (ImGui::MenuItem("Point Light"))
            {
                if (!m_Scene->m_Registry.try_get<PointLightComponent>(selectedEntity))
                    m_Scene->m_Registry.emplace<PointLightComponent>(selectedEntity);
            }
            if (ImGui::MenuItem("Directional Light"))
            {
                if (!m_Scene->m_Registry.try_get<DirectionalLightComponent>(selectedEntity))
                    m_Scene->m_Registry.emplace<DirectionalLightComponent>(selectedEntity);
            }
            if (ImGui::MenuItem("Sprite Animator"))
            {
                if (!m_Scene->m_Registry.try_get<SpriteAnimatorComponent>(selectedEntity))
                    m_Scene->m_Registry.emplace<SpriteAnimatorComponent>(selectedEntity);
            }
            ImGui::EndPopup();
        }

        // Transform Component
        if (auto* transform = m_Scene->m_Registry.try_get<TransformComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                DrawVec3Control("Position", transform->m_Position);
                DrawVec3Control("Rotation", transform->m_Rotation, 0.0f, 0.1f);
                DrawVec3Control("Scale", transform->m_Scale);
            }
        }

        // Static Mesh Component
        if (auto* mesh = m_Scene->m_Registry.try_get<StaticMeshComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("Static Mesh", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // Get current mesh info and setup colors
                std::string buttonLabel = "Drop Mesh Here";
                bool hasMesh = false;

                if (auto* currentMesh = AssetManager::Get().GetStaticMesh(mesh->m_AssetID))
                {
                    buttonLabel = currentMesh->GetPath().filename().string();
                    hasMesh = true;
                }

                // Drop target button with mesh name or default text
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

                // Color based on mesh presence
                if (hasMesh)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));  // Green for valid mesh
                else
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));  // Red for no mesh

                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

                ImGui::Button(buttonLabel.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 40));

                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();

                // Handle drag and drop
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_STATIC_MESH"))
                    {
                        PIX3D::UUID* draggedAssetId = (PIX3D::UUID*)payload->Data;
                        mesh->m_AssetID = *draggedAssetId;
                    }
                    ImGui::EndDragDropTarget();
                }

                // Only show Clear button if we have a mesh
                if (hasMesh)
                {
                    float clearButtonWidth = 60.0f;
                    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - clearButtonWidth);

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.3f, 1.0f));  // Unique color for clear
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.4f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.2f, 1.0f));

                    if (ImGui::Button("Clear", ImVec2(clearButtonWidth, 0)))
                        mesh->m_AssetID = 0;

                    ImGui::PopStyleColor(3);
                }
            }
        }

        // Sprite Component
        if (auto* sprite = m_Scene->m_Registry.try_get<SpriteComponent>(selectedEntity))
        {
            bool data_changed = false;
            if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::ColorEdit4("Color", &sprite->m_Material->m_Data->color.x))
                    data_changed = true;
                if (ImGui::DragFloat("Tiling Factor", &sprite->m_Material->m_Data->tiling_factor, 0.1f, 0.0f, 100.0f))
                    data_changed = true;

                // Texture preview and change button
                ImVec2 availableRegion = ImGui::GetContentRegionAvail();
                ImGui::Image((ImTextureID)sprite->m_Material->m_Texture->GetImGuiDescriptorSet(),
                    { 256.0f, 256.0f }, { 0, 0 }, { 1, 1 });

                if (ImGui::Button("Set Texture"))
                {
                    SpriteComponentChangeTexture = true;
                    data_changed = true;
                }

                ImGui::SameLine();
                bool flip = sprite->m_Material->m_Data->flip;
                if (ImGui::Checkbox("Flip", &flip))
                {
                    sprite->m_Material->m_Data->flip = flip;
                    data_changed = true;
                }


                if (data_changed)
                    sprite->m_Material->UpdateBuffer();
            }
        }

        // point light component
        if (auto* pointlight = m_Scene->m_Registry.try_get<PointLightComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::ColorEdit4("Color", &pointlight->m_Color.x);
                ImGui::SliderFloat("Intensity", &pointlight->m_Intensity, 0.0f, 100.0f);
                ImGui::SliderFloat("Raduis", &pointlight->m_Radius, 0.0f, 20.0f);
                ImGui::SliderFloat("FallOff", &pointlight->m_Falloff, 0.0f, 5.0f);
            }
        }

        if (auto* dirlight = m_Scene->m_Registry.try_get<DirectionalLightComponent>(selectedEntity))
        {
            auto* transform = m_Scene->m_Registry.try_get<TransformComponent>(selectedEntity);

            if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                /*
                if (ImGui::gizmo3D("##LightDirection", lightDirection, ImGui::GetFrameHeightWithSpacing() * 4, imguiGizmo::modeDirection)) {
                    // Normalize the direction vector after manipulation
                    lightDirection = vgm::normalize(lightDirection);

                    // Update your directional light or object's properties
                    dirlight->m_Direction = { lightDirection.x, lightDirection.y * -1.0f, lightDirection.z };
                }
                */

                ImGui::DragFloat3("Direction", &dirlight->m_Direction[0], 0.01f, -1.0f, 1.0f);
                ImGui::ColorEdit4("Color", &dirlight->m_Color.x);
                ImGui::SliderFloat("Intensity", &dirlight->m_Intensity, 0.0f, 10.0f);
            }
        }

        if (auto* animator = m_Scene->m_Registry.try_get<SpriteAnimatorComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("Sprite Animator", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool data_changed = false;

                // Texture preview
                if (animator->m_Material && animator->m_Material->GetTexture())
                {
                    ImGui::Image
                    (
                        (ImTextureID)animator->m_Material->GetTexture()->GetImGuiDescriptorSet(),
                        { 256.0f, 64.0f },
                        { 0, 0 },
                        { 1, 1 }
                    );
                }

                // Material properties
                if (ImGui::ColorEdit4("Color", &animator->m_Color.x))
                {
                    animator->m_Material->m_Data->color = animator->m_Color;
                    data_changed = true;
                }

                if (ImGui::DragFloat("Tiling Factor", &animator->m_TilingFactor, 0.1f, 0.0f, 100.0f))
                {
                    animator->m_Material->m_Data->tiling_factor = animator->m_TilingFactor;
                    data_changed = true;
                }

                if (ImGui::Checkbox("Flip", &animator->m_Flip))
                {
                    animator->m_Material->m_Data->flip = animator->m_Flip;
                    data_changed = true;
                }

                if (ImGui::Button("Set Sprite Sheet"))
                {
                    SpriteAnimatorComponentChangeTexture = true;
                    data_changed = true;
                }

                // Animation controls
                if (ImGui::DragInt("Frame Count", &animator->m_FrameCount, 1, 1, 32))
                {
                    animator->m_Material->m_Data->uv_scale = { 1.0f / animator->m_FrameCount, 1.0f };
                    data_changed = true;
                }

                ImGui::DragFloat("Frame Time", &animator->m_FrameTime, 0.01f, 0.01f, 1.0f, "%.3f");

                // Playback controls
                if (ImGui::Button(animator->m_IsPlaying ? "Pause" : "Play"))
                    animator->m_IsPlaying = !animator->m_IsPlaying;

                ImGui::SameLine();
                if (ImGui::Button("Reset"))
                {
                    animator->m_CurrentFrame = 0;
                    animator->m_CurrentTime = 0.0f;
                    animator->m_Material->m_Data->uv_offset.x = 0.0f;
                    data_changed = true;
                }

                ImGui::Checkbox("Loop", &animator->m_Loop);

                // Current frame display
                ImGui::Text("Current Frame: %d/%d", animator->m_CurrentFrame + 1, animator->m_FrameCount);
                ImGui::ProgressBar((float)animator->m_CurrentFrame / (float)(animator->m_FrameCount - 1));

                if (data_changed)
                    animator->m_Material->UpdateBuffer();
            }
        }
    }

    ImGui::End();
}

void InspectorWidget::PostFrameProcesses()
{

    auto selectedEntity = m_HierarchyWidget->GetSelectedEntity();

    if (selectedEntity != entt::null)
    {
        if (SpriteAnimatorComponentChangeTexture)
        {
            if (auto* animator = m_Scene->m_Registry.try_get<SpriteAnimatorComponent>(selectedEntity))
            {
                auto* platform = PIX3D::Engine::GetPlatformLayer();
                std::filesystem::path filepath = platform->OpenDialogue(PIX3D::FileDialougeFilter::PNG);
                if (!filepath.empty())
                {
                    auto* new_texture = new VK::VulkanTexture();
                    new_texture->LoadFromFile(filepath.string(), false, true);

                    // Update material's texture
                    animator->m_Material->ChangeTexture(new_texture);
                    
                    // Reset animation
                    animator->m_CurrentFrame = 0;
                    animator->m_CurrentTime = 0.0f;
                }

                SpriteAnimatorComponentChangeTexture = false;
            }
        }

        if (SpriteComponentChangeTexture)
        {
            if (auto* sprite = m_Scene->m_Registry.try_get<SpriteComponent>(selectedEntity))
            {
                auto* platform = PIX3D::Engine::GetPlatformLayer();
                std::filesystem::path filepath = platform->OpenDialogue(PIX3D::FileDialougeFilter::PNG);
                if (!filepath.empty())
                {
                    auto* new_texture = new VK::VulkanTexture();
                    new_texture->LoadFromFile(filepath.string(), false, true);
                    sprite->m_Material->ChangeTexture(new_texture);
                }

                SpriteComponentChangeTexture = false;
            }
        }
    }
}
