#pragma once
#include <Scene/Scene.h>
#include <glm/glm.hpp>

namespace PIX3D
{
    class ScriptGlue
    {
    public:
        static void SetScene(PIX3D::Scene* scene) { s_Scene = scene; }
        static void RegisterFunctions();

    private:

        inline static PIX3D::Scene* s_Scene;

        // Transform Component
        static void Transform_GetPosition(uint64_t entityID, glm::vec3* outPosition);
        static void Transform_SetPosition(uint64_t entityID, glm::vec3* position);
        static void Transform_GetRotation(uint64_t entityID, glm::vec3* outRotation);
        static void Transform_SetRotation(uint64_t entityID, glm::vec3* rotation);
        static void Transform_GetScale(uint64_t entityID, glm::vec3* outScale);
        static void Transform_SetScale(uint64_t entityID, glm::vec3* scale);
    };
}
