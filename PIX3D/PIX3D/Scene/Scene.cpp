#include "Scene.h"
#include <Platfrom/Vulkan/VulkanSystems.h>
#include <Engine/Engine.hpp>
#include <fstream>

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
        Material->Create(sprite.Texture);
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

    uint32_t Scene::AddSpriteAnimation(const std::string& name, const TransformData& transform, VK::VulkanTexture* spriteSheet, uint32_t frameCount, float frameTime)
    {
        const auto entity = m_Registry.create();
        m_Registry.emplace<TagComponent>(entity, name);
        m_Registry.emplace<TransformComponent>(entity, transform.Position, transform.Rotation, transform.Scale);
        auto& animatorComp = m_Registry.emplace<SpriteAnimatorComponent>(entity, spriteSheet, frameCount, frameTime);

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

    void Scene::CollectReferencedAssets()
    {
        m_ReferencedAssets.clear();

        // Collect all static mesh assets
        auto view = m_Registry.view<StaticMeshComponent>();
        for (auto entity : view)
        {
            auto& meshComp = view.get<StaticMeshComponent>(entity);
            if (meshComp.m_AssetID != 0)
            {
                m_ReferencedAssets.insert(meshComp.m_AssetID);
            }
        }
    }

    void Scene::SerializeReferencedAssets()
    {
        auto& project = PIX3D::Engine::GetCurrentProjectRef();
        auto assetPath = project.GetAssetsPath();

        // First collect all referenced assets
        CollectReferencedAssets();

        // Save each referenced asset
        for (const auto& assetID : m_ReferencedAssets)
        {
            if (auto* mesh = AssetManager::Get().GetStaticMesh(assetID))
            {
                // Create assets folder if it doesn't exist
                std::filesystem::create_directories(assetPath);

                // Copy the original file to project's asset folder
                std::filesystem::path originalPath = mesh->GetPath();
                std::filesystem::path destPath = assetPath / originalPath.filename();

                if (!std::filesystem::exists(destPath))
                {
                    try
                    {
                        std::filesystem::copy_file(originalPath, destPath,
                            std::filesystem::copy_options::overwrite_existing);
                    }
                    catch (const std::exception& e)
                    {
                        PIX_DEBUG_ERROR_FORMAT("Failed to copy asset file: {0}", e.what());
                    }
                }
            }
        }

        // Save asset manifest
        json assetManifest;
        assetManifest["assets"] = json::array();

        for (const auto& assetID : m_ReferencedAssets)
        {
            if (auto* mesh = AssetManager::Get().GetStaticMesh(assetID))
            {
                json assetEntry;
                assetEntry["uuid"] = (uint64_t)assetID;
                assetEntry["type"] = "StaticMesh";
                assetEntry["path"] = mesh->GetPath().filename().string();
                assetEntry["scale"] = mesh->m_Scale;
                assetManifest["assets"].push_back(assetEntry);
            }
        }

        // Save manifest to project
        std::ofstream manifestFile(assetPath / "asset_manifest.json");
        manifestFile << std::setw(4) << assetManifest << std::endl;
    }
}
