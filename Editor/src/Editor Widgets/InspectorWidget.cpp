#include "InspectorWidget.h"
//#include <imGuIZMO.quat/imGuIZMOquat.h>
#include <imgui.h>
#include <imgui_internal.h>

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
                // Add mesh properties like path, scale factor etc.
                if (ImGui::Button("Load Mesh"))
                {
                    auto* platform = PIX3D::Engine::GetPlatformLayer();
                    std::filesystem::path filepath = platform->OpenDialogue(PIX3D::FileDialougeFilter::GLTF);
                    if (!filepath.empty())
                    {
                        mesh->m_Mesh.Load(filepath.string(), 1.0f);
                    }
                }
            }
        }

        // Sprite Component
        if (auto* sprite = m_Scene->m_Registry.try_get<SpriteComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::ColorEdit4("Color", &sprite->m_Material->m_Data->color.x);
                ImGui::DragFloat("Tiling Factor", &sprite->m_Material->m_Data->tiling_factor, 0.1f, 0.0f, 100.0f);

                // Texture preview and change button
                ImVec2 availableRegion = ImGui::GetContentRegionAvail();
                if (sprite->m_Material->m_Data->use_texture)
                {
                    ImGui::Image((ImTextureID)sprite->m_Material->m_Texture->GetImGuiDescriptorSet(),
                        { 256.0f, 256.0f }, { 0, 0 }, { 1, 1 });
                }

                if (ImGui::Button("Set Texture"))
                {
                    auto* platform = PIX3D::Engine::GetPlatformLayer();
                    std::filesystem::path filepath = platform->OpenDialogue(PIX3D::FileDialougeFilter::PNG);
                    if (!filepath.empty())
                    {

                        if(sprite->m_Material->m_Texture)
                            sprite->m_Material->m_Texture->Destroy();

                        delete sprite->m_Material->m_Texture;
                        sprite->m_Material->m_Texture = new VK::VulkanTexture();
                        sprite->m_Material->m_Texture->LoadFromFile(filepath.string(), false, true);
                    }
                }
                ImGui::SameLine();
                bool flip = sprite->m_Material->m_Data->flip;
                if (ImGui::Checkbox("Flip", &flip))
                    sprite->m_Material->m_Data->flip = flip;
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
                // Texture preview and change button
                if (animator->m_SpriteSheet)
                {
                    ImGui::Image((ImTextureID)animator->m_SpriteSheet->GetImGuiDescriptorSet(),
                        { 256.0f, 64.0f }, { 0, 0 }, { 1, 1 });
                }

                ImGui::Checkbox("Flip", &animator->m_Flip);
                
                if (ImGui::Button("Set Sprite Sheet"))
                {
                    auto* platform = PIX3D::Engine::GetPlatformLayer();
                    std::filesystem::path filepath = platform->OpenDialogue(PIX3D::FileDialougeFilter::PNG);
                    if (!filepath.empty())
                    {
                        if (animator->m_SpriteSheet)
                            animator->m_SpriteSheet->Destroy();

                        delete animator->m_SpriteSheet;
                        animator->m_SpriteSheet = new VK::VulkanTexture();
                        animator->m_SpriteSheet->LoadFromFile(filepath.string(), false, true);
                    }
                }

                // Animation controls
                ImGui::ColorEdit4("Color", &animator->m_Color.x);
                ImGui::DragFloat("Tiling Factor", &animator->m_TilingFactor, 0.1f, 0.0f, 100.0f);
                ImGui::DragInt("Frame Count", &animator->m_FrameCount, 1, 1, 32);
                ImGui::DragFloat("Frame Time", &animator->m_FrameTime, 0.01f, 0.01f, 1.0f, "%.3f");

                // Playback controls
                if (ImGui::Button(animator->m_IsPlaying ? "Pause" : "Play"))
                    animator->m_IsPlaying = !animator->m_IsPlaying;

                ImGui::SameLine();
                if (ImGui::Button("Reset"))
                {
                    animator->m_CurrentFrame = 0;
                    animator->m_CurrentTime = 0.0f;
                }

                ImGui::Checkbox("Loop", &animator->m_Loop);

                // Current frame display
                ImGui::Text("Current Frame: %d/%d", animator->m_CurrentFrame + 1, animator->m_FrameCount);
                ImGui::ProgressBar((float)animator->m_CurrentFrame / (float)(animator->m_FrameCount - 1));
            }
        }
    }

    ImGui::End();
}
