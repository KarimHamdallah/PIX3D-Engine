#include "HierarchyWidget.h"
#include <imgui.h>

void HierarchyWidget::OnRender()
{
    ImGui::Begin("Hierarchy");

    // Create entity button
    if (ImGui::BeginPopupContextWindow("Create Entity", ImGuiPopupFlags_MouseButtonRight))
    {
        if (ImGui::MenuItem("Add Static Mesh"))
        {
            TransformData transform;
            m_Scene->AddStaticMesh("New Mesh", transform);
        }

        if (ImGui::MenuItem("Add Sprite"))
        {
            TransformData transform;
            SpriteData sprite;
            m_Scene->AddSprite("New Sprite", transform, sprite);
        }

        if (ImGui::MenuItem("Add Point Light"))
        {
            TransformData transform;
            m_Scene->AddPointLight("New Point Light", transform, {1.0f, 1.0f, 1.0f, 1.0f});
        }

        if (ImGui::MenuItem("Add Dir Light"))
        {
            TransformData transform;
            m_Scene->AddDirectionalLight("Dir Light", transform, { 1.0f, 1.0f, 1.0f, 1.0f });
        }

        if (ImGui::MenuItem("Add Sprite Animation"))
        {
            TransformData transform;
            m_Scene->AddSpriteAnimation("New Animation", transform, 0, 4, 0.05f); // Default 4 frames at 0.1s each
        }
        ImGui::EndPopup();
    }

    // Display entities
    auto view = m_Scene->m_Registry.view<TagComponent>();
    for (auto entity : view)
    {
        const auto& tag = view.get<TagComponent>(entity).m_Tag;

        ImGuiTreeNodeFlags flags = (m_SelectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf;

        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)entity, flags, tag.c_str());
        if (ImGui::IsItemClicked())
        {
            m_SelectedEntity = entity;

           m_Scene->SetSelectedEntityID(view.get<TagComponent>(entity).m_UUID);
        }

        if (opened)
            ImGui::TreePop();
    }

    ImGui::End();
}
