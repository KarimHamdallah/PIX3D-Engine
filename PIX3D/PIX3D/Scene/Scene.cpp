#include "Scene.h"
#include <Platfrom/Vulkan/VulkanSystems.h>

namespace PIX3D
{
    Scene::Scene(const std::string& name)
        : m_Name(name)
    {
    }

    uint32_t Scene::AddGameObject(const std::string& name, const TransformData& transform)
    {
        const auto entity = m_Registry.create();
        m_Registry.emplace<TagComponent>(entity, name);
        m_Registry.emplace<TransformComponent>(entity, transform.Position, transform.Rotation, transform.Scale);
        return (uint32_t)entity;
    }

    uint32_t Scene::AddStaticMesh(const std::string& name, const TransformData& transform, VulkanStaticMesh& mesh)
    {
        const auto entity = m_Registry.create();
        m_Registry.emplace<TagComponent>(entity, name);
        m_Registry.emplace<TransformComponent>(entity, transform.Position, transform.Rotation, transform.Scale);
        m_Registry.emplace<StaticMeshComponent>(entity, mesh);

        return (uint32_t)entity;
    }

    uint32_t Scene::AddSprite(const std::string& name, const TransformData& transform, const SpriteData& sprite)
    {
        const auto entity = m_Registry.create();
        m_Registry.emplace<TagComponent>(entity, name);
        m_Registry.emplace<TransformComponent>(entity, transform.Position, transform.Rotation, transform.Scale);


        SpriteMaterial* Material = new SpriteMaterial();
        Material->Create(sprite.Texture);
        Material->m_Data->color = sprite.Color;
        Material->m_Data->tiling_factor = sprite.TilingFactor;

        auto& spriteComp = m_Registry.emplace<SpriteComponent>(entity, Material);

        return (uint32_t)entity;
    }

    uint32_t Scene::AddPointLight(const std::string& name, const TransformData& transform, const glm::vec4& color)
    {
        const auto entity = m_Registry.create();
        m_Registry.emplace<TagComponent>(entity, name);
        m_Registry.emplace<TransformComponent>(entity, transform.Position, transform.Rotation, transform.Scale);
        m_Registry.emplace<PointLightComponent>(entity, color);
        return (uint32_t)entity;
    }

    uint32_t Scene::AddDirectionalLight(const std::string& name, const TransformData& transform, const glm::vec4& color)
    {
        const auto entity = m_Registry.create();
        m_Registry.emplace<TagComponent>(entity, name);
        m_Registry.emplace<TransformComponent>(entity, transform.Position, transform.Rotation, transform.Scale);
        m_Registry.emplace<DirectionalLightComponent>(entity, color);
        return (uint32_t)entity;
    }

    void Scene::OnStart()
    {
        // Initialize camera
        m_Cam3D.Init({ 0.0f, 0.0f, 5.0f });
    }

    void Scene::OnUpdate(float dt)
    {
        m_Cam3D.Update(dt);

        SpriteAnimatorSystem::Update(m_Registry, dt);
    }

    void Scene::OnRender()
    {
        VK::VulkanSceneRenderer::Begin(m_Cam3D);

        VK::VulkanSceneRenderer::RenderClearPass(m_BackgroundColor);

        if (m_UseSkybox)
            VK::VulkanSceneRenderer::RenderSkyBox();

        // Render static meshes
        StaticMeshRendererSystem::Render(m_Registry);

        // Render sprites
        SpriteRendererSystem::Render(m_Registry);

        // Render animated sprites
        SpriteAnimatorSystem::Render(m_Registry);

        VK::VulkanSceneRenderer::End();
    }
}
