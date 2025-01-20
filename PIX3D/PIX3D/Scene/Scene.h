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
        uint32_t AddSpriteAnimation(const std::string& name, const TransformData& transform, VK::VulkanTexture* spriteSheet, uint32_t frameCount, float frameTime);

        void OnStart();
        void OnUpdate(float dt);
        void OnRender();

        // Scene Properties
        bool m_UseSkybox = true;
        glm::vec4 m_BackgroundColor = { 0.05f, 0.05f, 0.05f, 1.0f };

        Camera3D m_Cam3D;

        // Registry access
        entt::registry& GetRegistry() { return m_Registry; }

        // New functions for asset management
        void CollectReferencedAssets();
        void SerializeReferencedAssets();
        void LoadReferencedAssets();

    public:
        std::string m_Name;
        entt::registry m_Registry;

        uint32_t m_PointLightsCount = 0;
        std::vector<_PointLightShaderData> m_PointLightsData;
        VK::VulkanShaderStorageBuffer m_PointLightsShaderBuffer;
        VK::VulkanDescriptorSet PointLightsDescriptorSet;

        VulkanStaticMesh m_LightBulbMesh;

        std::unordered_set<uint64_t> m_ReferencedAssets;
        friend class SceneSerializer;
    };
}
