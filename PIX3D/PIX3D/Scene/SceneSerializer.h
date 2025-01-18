#include <Utils/json.hpp>
#include "Scene.h"
#include <fstream>

using json = nlohmann::json;

namespace PIX3D
{
    class SceneSerializer
    {
    public:
        SceneSerializer(Scene* scene)
            : m_Scene(scene)
        {
        }

        void Serialize(const std::string& filepath)
        {
            json j;

            // Scene properties
            j["name"] = m_Scene->m_Name;
            j["backgroundColor"] = {
                m_Scene->m_BackgroundColor.r,
                m_Scene->m_BackgroundColor.g,
                m_Scene->m_BackgroundColor.b,
                m_Scene->m_BackgroundColor.a
            };
            j["useSkybox"] = m_Scene->m_UseSkybox;

            // Serialize entities
            j["entities"] = json::array();

            auto view = m_Scene->m_Registry.view<TagComponent>();
            for (auto entity : view)
            {
                json entityJson;
                SerializeEntity(entity, entityJson);
                j["entities"].push_back(entityJson);
            }

            // Write to file
            std::ofstream file(filepath);
            file << std::setw(4) << j << std::endl;
        }

        bool Deserialize(const std::string& filepath)
        {
            try
            {
                std::ifstream file(filepath);
                if (!file.is_open())
                    return false;

                json j;
                file >> j;

                // Clear existing scene
                m_Scene->m_Registry.clear();

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
                // Log error here
                return false;
            }
        }

    private:
        void SerializeEntity(entt::entity entity, json& outJson)
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
                    {"filepath", staticMesh->m_Mesh.m_Path.string()},
                    {"scale", staticMesh->m_Mesh.m_Scale }
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
                    {"texturePath", sprite->m_Material->GetTexture()->GetPath().string()}
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
                    {"spriteSheetPath", spriteAnim->m_Material->m_Texture->GetPath().string()},
                    {"frameCount", spriteAnim->m_FrameCount},
                    {"frameTime", spriteAnim->m_FrameTime}
                };
            }
        }

        void DeserializeEntity(const json& entityJson)
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
                VulkanStaticMesh mesh;
                mesh.Load(meshData["filepath"], meshData["scale"]);
                entity = m_Scene->AddStaticMesh(tag, transform, mesh);
            }
            else if (type == "Sprite")
            {
                const auto& spriteData = entityJson["sprite"];
                const auto& color = spriteData["color"];
                SpriteData sprite;
                sprite.Color = { color[0], color[1], color[2], color[3] };
                sprite.TilingFactor = spriteData["tilingFactor"];

                VK::VulkanTexture* texture = new VK::VulkanTexture();
                texture->LoadFromFile(spriteData["texturePath"], true);
                sprite.Texture = texture;

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
                VK::VulkanTexture* spriteSheet = new VK::VulkanTexture();
                spriteSheet->LoadFromFile(animData["spriteSheetPath"], true);

                entity = m_Scene->AddSpriteAnimation(
                    tag,
                    transform,
                    spriteSheet,
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

    private:
        Scene* m_Scene;
    };
}
