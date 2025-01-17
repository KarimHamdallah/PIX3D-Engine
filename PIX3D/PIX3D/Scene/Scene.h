#pragma once
#include <string>
#include <Utils/entt.hpp>
#include <Graphics/Camera3D.h>
#include <Graphics/VulkanStaticMesh.h>
#include <Platfrom/Vulkan/VulkanHdrCubemap.h>
#include "Components.h"

#include "SceneStructures.h"

namespace PIX3D
{
    class Scene
    {
    public:
        Scene(const std::string& name = "Scene");
        ~Scene() = default;

        // Entity Management
        uint32_t AddGameObject(const std::string& name, const TransformData& transform);
        uint32_t AddStaticMesh(const std::string& name, const TransformData& transform, VulkanStaticMesh& mesh);
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

    public:
        std::string m_Name;
        entt::registry m_Registry;

        friend class SceneSerializer;
    };
}
