#include "SceneSerializer.h"
#include "Engine/Engine.hpp"
#include "Asset/AssetManager.h"

namespace PIX3D
{
    void SceneSerializer::SerializeReferencedAssets(const std::string& filepath)
    {
        // Clear existing references
        m_ReferencedAssets.clear();

        // Collect all static mesh assets
        auto view = m_Scene->m_Registry.view<StaticMeshComponent>();
        for (auto entity : view)
        {
            auto& meshComp = view.get<StaticMeshComponent>(entity);
            if (meshComp.m_AssetID != PIX3D::UUID(0))
            {
                m_ReferencedAssets.insert(meshComp.m_AssetID);
            }
        }

        // Collect all sprite textures
        auto spriteView = m_Scene->m_Registry.view<SpriteComponent>();
        for (auto entity : spriteView)
        {
            auto& spriteComp = spriteView.get<SpriteComponent>(entity);
            if (spriteComp.m_Material->m_TextureUUID != PIX3D::UUID(0))
            {
                m_ReferencedAssets.insert(spriteComp.m_Material->m_TextureUUID);
            }
        }

        // Collect all sprite animator textures
        auto animView = m_Scene->m_Registry.view<SpriteAnimatorComponent>();
        for (auto entity : animView)
        {
            auto& animComp = animView.get<SpriteAnimatorComponent>(entity);
            if (animComp.m_Material->m_TextureUUID != PIX3D::UUID(0))
            {
                m_ReferencedAssets.insert(animComp.m_Material->m_TextureUUID);
            }
        }

        // Get project asset path
        auto& project = Engine::GetCurrentProjectRef();
        auto assetPath = project.GetAssetsPath();
        
        //PIX_ASSERT(project.IsValid());
        if (!project.IsValid())
        {
            assetPath = std::filesystem::path(filepath).parent_path().string();
        }


        // Create asset manifest
        json assetManifest;
        assetManifest["assets"] = json::array();

        // Save each referenced asset
        for (const auto& assetID : m_ReferencedAssets)
        {
            json assetEntry;
            bool validAsset = false;

            // Try as mesh
            if (auto* mesh = AssetManager::Get().GetStaticMesh(assetID))
            {
                assetEntry["uuid"] = (uint64_t)assetID;
                assetEntry["type"] = "StaticMesh";
                assetEntry["path"] = mesh->GetPath().string();
                assetEntry["scale"] = mesh->m_Scale;
                validAsset = true;
            }
            // Try as texture
            else if (auto* texture = AssetManager::Get().GetTexture(assetID))
            {
                assetEntry["uuid"] = (uint64_t)assetID;
                assetEntry["type"] = "Texture";
                assetEntry["path"] = texture->GetPath().string();
                validAsset = true;
            }

            if (validAsset)
            {
                assetManifest["assets"].push_back(assetEntry);
            }
        }

        // Save manifest
        std::ofstream manifestFile(assetPath / "asset_manifest.json");
        manifestFile << std::setw(4) << assetManifest << std::endl;
    }

    void SceneSerializer::LoadReferencedAssets(const std::string& filepath)
    {
        auto& project = Engine::GetCurrentProjectRef();
        auto assetPath = project.GetAssetsPath();

        //PIX_ASSERT(project.IsValid());
        if (!project.IsValid())
        {
            assetPath = std::filesystem::path(filepath).parent_path().string();
        }
        auto manifestPath = assetPath / "asset_manifest.json";

        try
        {
            std::ifstream manifestFile(manifestPath);
            json manifest;
            manifestFile >> manifest;

            for (const auto& assetEntry : manifest["assets"])
            {
                PIX3D::UUID uuid = assetEntry["uuid"].get<uint64_t>();
                std::string type = assetEntry["type"].get<std::string>();
                std::filesystem::path assetFilePath = assetEntry["path"].get<std::string>();

                if (type == "StaticMesh" && FileExists(assetFilePath.string()))
                {
                    float scale = assetEntry["scale"].get<float>();
                    AssetManager::Get().LoadStaticMeshUUID(uuid, assetFilePath.string(), scale);
                }
                else if (type == "Texture" && FileExists(assetFilePath.string()))
                {
                    AssetManager::Get().LoadTextureUUID(uuid, assetFilePath.string(), true, true);
                }
            }
        }
        catch (const std::exception& e)
        {
            PIX_DEBUG_ERROR_FORMAT("Failed to load asset manifest: {0}", e.what());
        }
    }

    void SceneSerializer::Serialize(const std::string& filepath)
    {
        // First save referenced assets
        SerializeReferencedAssets(filepath);

        json j;
        j["name"] = m_Scene->m_Name;
        j["backgroundColor"] = {
            m_Scene->m_BackgroundColor.r,
            m_Scene->m_BackgroundColor.g,
            m_Scene->m_BackgroundColor.b,
            m_Scene->m_BackgroundColor.a
        };
        j["useSkybox"] = m_Scene->m_UseSkybox;

        j["entities"] = json::array();
        auto view = m_Scene->m_Registry.view<TagComponent>();
        for (auto entity : view)
        {
            json entityJson;
            SerializeEntity(entity, entityJson);
            j["entities"].push_back(entityJson);
        }

        std::ofstream file(filepath);
        file << std::setw(4) << j << std::endl;
    }

    bool SceneSerializer::Deserialize(const std::string& filepath)
    {
        try
        {
            std::ifstream file(filepath);
            if (!file.is_open()) return false;

            json j;
            file >> j;

            // Clear existing scene
            m_Scene->m_Registry.clear();

            // Try to load referenced assets first, but continue even if it fails
            LoadReferencedAssets(filepath);

            // Load scene properties
            m_Scene->m_Name = j["name"].get<std::string>();
            auto bgColor = j["backgroundColor"];
            m_Scene->m_BackgroundColor = {
                bgColor[0], bgColor[1], bgColor[2], bgColor[3]
            };
            m_Scene->m_UseSkybox = j["useSkybox"];

            // Load entities
            for (const auto& entityJson : j["entities"])
            {
                DeserializeEntity(entityJson);
            }

            return true;
        }
        catch (const std::exception& e)
        {
            PIX_DEBUG_ERROR_FORMAT("Failed to deserialize scene: {0}", e.what());
            return false;
        }
    }

    void SceneSerializer::SerializeEntity(entt::entity entity, json& outJson)
    {
        const auto& tag = m_Scene->m_Registry.get<TagComponent>(entity);
        outJson["tag"] = tag.m_Tag;

        if (auto* transform = m_Scene->m_Registry.try_get<TransformComponent>(entity))
        {
            outJson["transform"] = {
                {"position", {transform->m_Position.x, transform->m_Position.y, transform->m_Position.z}},
                {"rotation", {transform->m_Rotation.x, transform->m_Rotation.y, transform->m_Rotation.z}},
                {"scale", {transform->m_Scale.x, transform->m_Scale.y, transform->m_Scale.z}}
            };
        }

        if (auto* staticMesh = m_Scene->m_Registry.try_get<StaticMeshComponent>(entity))
        {
            outJson["type"] = "StaticMesh";
            outJson["staticMesh"] = {
                {"asset_id", (uint64_t)staticMesh->m_AssetID}
            };
        }

        if (auto* sprite = m_Scene->m_Registry.try_get<SpriteComponent>(entity))
        {
            outJson["type"] = "Sprite";
            outJson["sprite"] = {
                {"color", {
                    sprite->m_Material->m_Data->color.r,
                    sprite->m_Material->m_Data->color.g,
                    sprite->m_Material->m_Data->color.b,
                    sprite->m_Material->m_Data->color.a
                }},
                {"tilingFactor", sprite->m_Material->m_Data->tiling_factor},
                {"asset_id", (uint64_t)sprite->m_Material->m_TextureUUID}
            };
        }

        if (auto* pointLight = m_Scene->m_Registry.try_get<PointLightComponent>(entity))
        {
            outJson["type"] = "PointLight";
            outJson["pointLight"] = {
                {"color", {
                    pointLight->m_Color.r,
                    pointLight->m_Color.g,
                    pointLight->m_Color.b,
                    pointLight->m_Color.a
                }},
                {"radius", pointLight->m_Radius},
                {"intensity", pointLight->m_Intensity},
                {"falloff", pointLight->m_Falloff}
            };
        }

        if (auto* dirLight = m_Scene->m_Registry.try_get<DirectionalLightComponent>(entity))
        {
            outJson["type"] = "DirectionalLight";
            outJson["directionalLight"] = {
                {"color", {
                    dirLight->m_Color.r,
                    dirLight->m_Color.g,
                    dirLight->m_Color.b,
                    dirLight->m_Color.a
                }}
            };
        }

        if (auto* spriteAnim = m_Scene->m_Registry.try_get<SpriteAnimatorComponent>(entity))
        {
            outJson["type"] = "SpriteAnimation";
            outJson["spriteAnimation"] = {
                {"asset_id", (uint64_t)spriteAnim->m_Material->m_TextureUUID},
                {"frameCount", spriteAnim->m_FrameCount},
                {"frameTime", spriteAnim->m_FrameTime}
            };
        }
    }

    void SceneSerializer::DeserializeEntity(const json& entityJson)
    {
        std::string type = entityJson.value("type", "");
        std::string tag = entityJson["tag"];

        // Parse transform data
        TransformData transform;
        if (entityJson.contains("transform"))
        {
            const auto& t = entityJson["transform"];
            const auto& pos = t["position"];
            const auto& rot = t["rotation"];
            const auto& scale = t["scale"];

            transform.Position = { pos[0], pos[1], pos[2] };
            transform.Rotation = { rot[0], rot[1], rot[2] };
            transform.Scale = { scale[0], scale[1], scale[2] };
        }

        uint32_t entity = 0;

        if (type == "StaticMesh")
        {
            const auto& meshData = entityJson["staticMesh"];
            const uint64_t asset_id = meshData["asset_id"];
            entity = m_Scene->AddGameObject(tag, transform);
            m_Scene->m_Registry.emplace<StaticMeshComponent>((entt::entity)entity, PIX3D::UUID(asset_id));
        }
        else if (type == "Sprite")
        {
            const auto& spriteData = entityJson["sprite"];
            const auto& color = spriteData["color"];

            SpriteData sprite;
            sprite.Color = { color[0], color[1], color[2], color[3] };
            sprite.TilingFactor = spriteData["tilingFactor"];
            int64_t texture_uuid = spriteData["asset_id"];
            sprite.TextureUUID = texture_uuid;

            entity = m_Scene->AddSprite(tag, transform, sprite);
        }
        else if (type == "PointLight")
        {
            const auto& lightData = entityJson["pointLight"];
            const auto& color = lightData["color"];
            glm::vec4 lightColor = { color[0], color[1], color[2], color[3] };

            entity = m_Scene->AddPointLight(tag, transform, lightColor);

            if (auto* light = m_Scene->m_Registry.try_get<PointLightComponent>((entt::entity)entity))
            {
                light->m_Radius = lightData["radius"];
                light->m_Intensity = lightData["intensity"];
                light->m_Falloff = lightData["falloff"];
            }
        }
        else if (type == "DirectionalLight")
        {
            const auto& lightData = entityJson["directionalLight"];
            const auto& color = lightData["color"];
            glm::vec4 lightColor = { color[0], color[1], color[2], color[3] };

            entity = m_Scene->AddDirectionalLight(tag, transform, lightColor);
        }
        else if (type == "SpriteAnimation")
        {
            const auto& animData = entityJson["spriteAnimation"];
            uint64_t texture_uuid = animData["asset_id"];

            entity = m_Scene->AddSpriteAnimation(
                tag,
                transform,
                texture_uuid,
                animData["frameCount"],
                animData["frameTime"]
            );
        }
        else
        {
            // Default game object
            entity = m_Scene->AddGameObject(tag, transform);
        }
    }
}
