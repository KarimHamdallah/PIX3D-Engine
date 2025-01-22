#include "Scene.h"
#include <Platfrom/Vulkan/VulkanSystems.h>
#include <Engine/Engine.hpp>
#include <fstream>
#include <Scripting Engine/ScriptingEngine.h>

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

    uint32_t Scene::AddStaticMesh(const std::string& name, const TransformData& transform, PIX3D::UUID asset_id)
    {
        const auto entity = m_Registry.create();
        m_Registry.emplace<TagComponent>(entity, name);
        m_Registry.emplace<TransformComponent>(entity, transform.Position, transform.Rotation, transform.Scale);
        m_Registry.emplace<StaticMeshComponent>(entity, asset_id);

        return (uint32_t)entity;
    }

    uint32_t Scene::AddSprite(const std::string& name, const TransformData& transform, const SpriteData& sprite)
    {
        const auto entity = m_Registry.create();
        m_Registry.emplace<TagComponent>(entity, name);
        m_Registry.emplace<TransformComponent>(entity, transform.Position, transform.Rotation, transform.Scale);


        SpriteMaterial* Material = new SpriteMaterial();
        Material->Create(sprite.TextureUUID);
        Material->m_Data->color = sprite.Color;
        Material->m_Data->tiling_factor = sprite.TilingFactor;
        Material->UpdateBuffer();

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

    uint32_t Scene::AddSpriteAnimation(const std::string& name, const TransformData& transform, PIX3D::UUID spriteSheet, uint32_t frameCount, float frameTime)
    {
        const auto entity = m_Registry.create();
        m_Registry.emplace<TagComponent>(entity, name);
        m_Registry.emplace<TransformComponent>(entity, transform.Position, transform.Rotation, transform.Scale);
        auto& animatorComp = m_Registry.emplace<SpriteAnimatorComponent>(entity, spriteSheet, frameCount, frameTime);

        return (uint32_t)entity;
    }

    uint32_t Scene::AddScript(const std::string& name, const TransformData& transform, const std::string& NameSpaceName, const std::string& ScriptClassName)
    {
        const auto entity = m_Registry.create();
        m_Registry.emplace<TagComponent>(entity, name);
        m_Registry.emplace<TransformComponent>(entity, transform.Position, transform.Rotation, transform.Scale);
        m_Registry.emplace<ScriptComponentCSharp>(entity, NameSpaceName, ScriptClassName);
        return (uint32_t)entity;
    }

    void Scene::OnStart()
    {
        // Initialize camera
        m_Cam3D.Init({ 0.0f, 0.0f, 5.0f });

        //////////////////// Point Lights Data ///////////////////////////
        {
            m_LightBulbMesh.Load("res/led_light_bulb_9w/scene.gltf", 0.02f);

            m_PointLightsShaderBuffer.Create(MAX_POINT_LIGHTS * sizeof(_PointLightShaderData));

            m_PointLightsData.resize(MAX_POINT_LIGHTS);

            PointLightsDescriptorSet
                .Init(VK::VulkanSceneRenderer::s_PointLightsDescriptorSetLayout)
                .AddShaderStorageBuffer(0, m_PointLightsShaderBuffer)
                .Build();
        }
    }

    void Scene::OnUpdate(float dt)
    {
        m_Cam3D.Update(dt);

        SpriteAnimatorSystem::Update(m_Registry, dt);


        ///////////// Update Point Lights Shader Buffer ////////////////
        {
            int light_index = 0;
            auto view = m_Registry.view<TransformComponent, PointLightComponent>();
            view.each([this, &light_index](TransformComponent& transform, PointLightComponent& light)
                {
                    _PointLightShaderData light_data;
                    light_data.LightColor = light.m_Color;
                    light_data.LightPosition = { transform.m_Position.x, transform.m_Position.y, transform.m_Position.z, 1.0f };
                    light_data.Radius = light.m_Radius;
                    light_data.Intensity = light.m_Intensity;
                    light_data.Falloff = light.m_Falloff;

                    m_PointLightsData[light_index++] = light_data;
                });
            m_PointLightsCount = light_index;
            m_PointLightsShaderBuffer.UpdateData(m_PointLightsData.data(), MAX_POINT_LIGHTS * sizeof(_PointLightShaderData));
        }
    }

    void Scene::OnRender()
    {
        VK::VulkanSceneRenderer::Begin(m_Cam3D);

        VK::VulkanSceneRenderer::RenderClearPass(m_BackgroundColor);

        if (m_UseSkybox)
            VK::VulkanSceneRenderer::RenderSkyBox();

        // Render static meshes
        StaticMeshRendererSystem::Render(this);

        // Render sprites
        SpriteRendererSystem::Render(m_Registry);

        // Render animated sprites
        SpriteAnimatorSystem::Render(m_Registry);

        // Render Point Lights
        {
            auto view = m_Registry.view<TransformComponent, PointLightComponent>();
            view.each([this](TransformComponent& transform, PointLightComponent& light)
                {
                    VK::VulkanSceneRenderer::RenderMesh(this, m_LightBulbMesh, transform.GetTransformMatrix());
                });
        }

        VK::VulkanSceneRenderer::End();
    }

    void Scene::OnRunTimeStart()
    {
        OnScriptCreate();
    }

    void Scene::OnRunTimeUpdate(float dt)
    {
        // update scripts
        OnScriptUpdate(dt);
        
        // update scene
        OnUpdate(dt);
    }

    void Scene::OnRunTimeRender()
    {
        OnRender();
    }

    void Scene::OnRunTimeEnd()
    {
        OnScriptDestroy();
    }

    void Scene::OnScriptCreate()
    {
        auto view = m_Registry.view<TagComponent, ScriptComponentCSharp>();

        view.each([](const TagComponent& tag, ScriptComponentCSharp& scriptComp)
            {
                ScriptEngine::OnCreateEntity(&scriptComp, tag.m_UUID);
            });
    }

    void Scene::OnScriptUpdate(float dt)
    {
        auto view = m_Registry.view<TagComponent, ScriptComponentCSharp>();

        view.each([dt](const TagComponent& tag, ScriptComponentCSharp& scriptComp)
            {
                ScriptEngine::OnUpdateEntity(&scriptComp, tag.m_UUID, dt);
            });
    }

    void Scene::OnScriptDestroy()
    {
        auto view = m_Registry.view<TagComponent, ScriptComponentCSharp>();
        view.each([](const TagComponent& tag, ScriptComponentCSharp& scriptComp)
            {
                ScriptEngine::OnDestroyEntity(&scriptComp, tag.m_UUID);
            });
    }
}
