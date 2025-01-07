#pragma once
#include <PIX3D.h>

using namespace PIX3D;

class HierarchyWidget
{
public:
    HierarchyWidget(Scene* scene)
        : m_Scene(scene), m_SelectedEntity(entt::null) {}

    void OnRender();
    entt::entity GetSelectedEntity() const { return m_SelectedEntity; }

private:
    Scene* m_Scene;
    entt::entity m_SelectedEntity;
};
