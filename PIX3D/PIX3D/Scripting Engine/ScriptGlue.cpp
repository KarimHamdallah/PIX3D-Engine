#include "ScriptGlue.h"
#include "ScriptingEngine.h"
#include <mono/metadata/object.h>

namespace PIX3D
{
    void ScriptGlue::RegisterFunctions()
    {
        // Register transform component methods
        mono_add_internal_call("PIX3D.PIXEntity::GetPosition", (void*)Transform_GetPosition);
        mono_add_internal_call("PIX3D.PIXEntity::SetPosition", (void*)Transform_SetPosition);
        mono_add_internal_call("PIX3D.PIXEntity::GetRotation", (void*)Transform_GetRotation);
        mono_add_internal_call("PIX3D.PIXEntity::SetRotation", (void*)Transform_SetRotation);
        mono_add_internal_call("PIX3D.PIXEntity::GetScale", (void*)Transform_GetScale);
        mono_add_internal_call("PIX3D.PIXEntity::SetScale", (void*)Transform_SetScale);
    }

    void ScriptGlue::Transform_GetPosition(uint64_t entityID, glm::vec3* outPosition)
    {
        entt::entity entityHandle = s_Scene->GetEntityHandleByUUID(entityID);
        if (auto* transform = s_Scene->m_Registry.try_get<TransformComponent>(entityHandle))
        {
            *outPosition = transform->m_Position;
        }
    }

    void ScriptGlue::Transform_SetPosition(uint64_t entityID, glm::vec3* position)
    {
        entt::entity entityHandle = s_Scene->GetEntityHandleByUUID(entityID);
        if (auto* transform = s_Scene->m_Registry.try_get<TransformComponent>(entityHandle))
        {
            transform->m_Position = *position;
        }
    }

    void ScriptGlue::Transform_GetRotation(uint64_t entityID, glm::vec3* outRotation)
    {
        entt::entity entityHandle = s_Scene->GetEntityHandleByUUID(entityID);
        if (auto* transform = s_Scene->m_Registry.try_get<TransformComponent>(entityHandle))
        {
            *outRotation = transform->m_Rotation;
        }
    }

    void ScriptGlue::Transform_SetRotation(uint64_t entityID, glm::vec3* rotation)
    {
        entt::entity entityHandle = s_Scene->GetEntityHandleByUUID(entityID);
        if (auto* transform = s_Scene->m_Registry.try_get<TransformComponent>(entityHandle))
        {
            transform->m_Rotation = *rotation;
        }
    }

    void ScriptGlue::Transform_GetScale(uint64_t entityID, glm::vec3* outScale)
    {
        entt::entity entityHandle = s_Scene->GetEntityHandleByUUID(entityID);
        if (auto* transform = s_Scene->m_Registry.try_get<TransformComponent>(entityHandle))
        {
            *outScale = transform->m_Scale;
        }
    }

    void ScriptGlue::Transform_SetScale(uint64_t entityID, glm::vec3* scale)
    {
        entt::entity entityHandle = s_Scene->GetEntityHandleByUUID(entityID);
        if (auto* transform = s_Scene->m_Registry.try_get<TransformComponent>(entityHandle))
        {
            transform->m_Scale = *scale;
        }
    }
}
