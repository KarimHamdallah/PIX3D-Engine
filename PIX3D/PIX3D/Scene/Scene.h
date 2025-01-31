#pragma once
#include <string>
#include <Utils/entt.hpp>
#include <Graphics/Camera3D.h>
#include <Graphics/VulkanStaticMesh.h>
#include <Platfrom/Vulkan/VulkanHdrCubemap.h>
#include "Components.h"

#include "SceneStructures.h"
#include <vector>
#include <unordered_set>

#define MAX_POINT_LIGHTS 200

namespace PIX3D
{
    struct _PointLightShaderData
    {
        glm::vec4 LightPosition;
        glm::vec4 LightColor;
        float Intensity;  // Controls the brightness of the light
        float Radius;     // Controls the maximum range of the light
        float Falloff;      // Controls the falloff curve
    };

    class Scene
    {
    public:
        Scene(const std::string& name = "Scene");
        ~Scene() = default;

        // Entity Management
        uint32_t AddGameObject(const std::string& name, const TransformData& transform);
        uint32_t AddStaticMesh(const std::string& name, const TransformData& transform, PIX3D::UUID asset_id = 0);
        uint32_t AddSprite(const std::string& name, const TransformData& transform, const SpriteData& sprite);
        uint32_t AddPointLight(const std::string& name, const TransformData& transform, const glm::vec4& color);
        uint32_t AddDirectionalLight(const std::string& name, const TransformData& transform, const glm::vec4& color);
        uint32_t AddSpriteAnimation(const std::string& name, const TransformData& transform, PIX3D::UUID spriteSheet, uint32_t frameCount, float frameTime);
        uint32_t AddScript(const std::string& name, const TransformData& transform, const std::string& NameSpaceName, const std::string& ScriptClassName);

        void OnStart();
        void OnUpdate(float dt);
        void OnRender(bool end = false);


        void OnRunTimeStart();
        void OnRunTimeUpdate(float dt);
        void OnRunTimeRender();
        void OnRunTimeEnd();

        void OnScriptCreate();
        void OnScriptUpdate(float dt);
        void OnScriptDestroy();

        entt::entity GetEntityHandleByUUID(PIX3D::UUID uuid);

        static Scene* CopyScene(Scene* sourceScene);

        void SetSelectedEntityID(PIX3D::UUID entity_id);
        bool IsSelectedEntity(PIX3D::UUID entity_id);

        // Scene Properties
        bool m_UseSkybox = true;
        glm::vec4 m_BackgroundColor = { 0.05f, 0.05f, 0.05f, 1.0f };

        Camera3D m_Cam3D;

        // Registry access
        entt::registry& GetRegistry() { return m_Registry; }
    public:
        std::string m_Name;
        entt::registry m_Registry;

        uint32_t m_PointLightsCount = 0;
        std::vector<_PointLightShaderData> m_PointLightsData;
        VK::VulkanShaderStorageBuffer m_PointLightsShaderBuffer;
        VK::VulkanDescriptorSet PointLightsDescriptorSet;

        PIX3D::UUID m_SelectedEntityID = 0;

        VulkanStaticMesh m_LightBulbMesh;
        friend class SceneSerializer;
    };
}
