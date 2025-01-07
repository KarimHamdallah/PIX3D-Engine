#include "SceneSerializer.h"
#include <fstream>

#include <Utils/json.hpp>
using namespace nlohmann;

namespace PIX3D
{
	//////////////////////////////////////////
	//////// Serialize Componenets ///////////
	//////////////////////////////////////////

	void to_json(json& j, const PIX3D::TagComponent& component)
	{
		j = json{ {"Tag", component.m_Tag} };
	}

	void from_json(const json& j, PIX3D::TagComponent& component)
	{
		component.m_Tag = j.at("Tag").get<std::string>();
	}

	void to_json(json& j, const PIX3D::TransformComponent& component)
	{
		j = json{
			{"Position", {component.m_Position.x, component.m_Position.y, component.m_Position.z}},
			{"Rotation", {component.m_Rotation.x, component.m_Rotation.y, component.m_Rotation.z}},
			{"Scale", {component.m_Scale.x, component.m_Scale.y, component.m_Scale.z}}
		};
	}

	void from_json(const json& j, PIX3D::TransformComponent& component)
	{
		auto pos = j.at("Position").get<std::vector<float>>();
		auto rot = j.at("Rotation").get<std::vector<float>>();
		auto scale = j.at("Scale").get<std::vector<float>>();
		component.m_Position = { pos[0], pos[1], pos[2] };
		component.m_Rotation = { rot[0], rot[1], rot[2] };
		component.m_Scale = { scale[0], scale[1], scale[2] };
	}

    void to_json(json& j, const SpriteComponent& component)
    {
        j = json{
            {"TexturePath", component.m_Texture.GetPath().string()}, // Assuming the texture ID or a unique identifier
            {"Color", {component.m_Color.r, component.m_Color.g, component.m_Color.b, component.m_Color.a}},
            {"TilingFactor", component.m_TilingFactor},
            {"Flip", component.flip}
        };
    }

    void from_json(const json& j, SpriteComponent& component)
    {
        auto color = j.at("Color").get<std::vector<float>>();
        auto texturepath = j.at("TexturePath").get<std::string>();

        component.m_Texture.LoadFromFile(texturepath, true);
        component.m_Color = { color[0], color[1], color[2], color[3] };
        component.m_TilingFactor = j.at("TilingFactor").get<float>();
        component.flip = j.at("Flip").get<bool>();
        // Deserialize TextureID as needed; ensure texture manager handles it
    }

    void to_json(json& j, const StaticMeshComponent& component)
    {
        j = json{
            {"MeshPath", component.m_Mesh.m_Path.string()}
        };
    }

    void from_json(const json& j, StaticMeshComponent& component)
    {
        std::string meshpath = j.at("MeshPath").get<std::string>();
        
        // Retrieve the mesh using the ID from your asset manager
        component.m_Mesh.Load(meshpath);
    }

    void to_json(json& j, const PointLightComponent& component)
    {
        j = json{
            {"Color", {component.m_Color.r, component.m_Color.g, component.m_Color.b, component.m_Color.a}},
            {"Intensity", component.m_Intensity},
            {"Radius", component.m_Raduis},
            {"Falloff", component.m_Falloff}
        };
    }

    void from_json(const json& j, PointLightComponent& component)
    {
        auto color = j.at("Color").get<std::vector<float>>();
        component.m_Color = { color[0], color[1], color[2], color[3] };
        component.m_Intensity = j.at("Intensity").get<float>();
        component.m_Raduis = j.at("Radius").get<float>();
        component.m_Falloff = j.at("Falloff").get<float>();
    }

    void to_json(json& j, const DirectionalLightComponent& component)
    {
        j = json{
            {"Direction", {component.m_Direction.x, component.m_Direction.y, component.m_Direction.z}},
            {"Color", {component.m_Color.r, component.m_Color.g, component.m_Color.b, component.m_Color.a}},
            {"Intensity", component.m_Intensity}
        };
    }

    void from_json(const json& j, DirectionalLightComponent& component) {
        auto direction = j.at("Direction").get<std::vector<float>>();
        auto color = j.at("Color").get<std::vector<float>>();
        component.m_Direction = { direction[0], direction[1], direction[2] };
        component.m_Color = { color[0], color[1], color[2], color[3] };
        component.m_Intensity = j.at("Intensity").get<float>();
    }

    void to_json(json& j, const SpriteAnimatorComponent& component)
    {
        j = json{
            {"SpriteSheetPath", component.m_SpriteSheet.GetPath().string()}, // Assuming the sprite sheet has an ID
            {"FrameCount", component.m_FrameCount},
            {"FrameTime", component.m_FrameTime},
            {"CurrentTime", component.m_CurrentTime},
            {"CurrentFrame", component.m_CurrentFrame},
            {"IsPlaying", component.m_IsPlaying},
            {"Loop", component.m_Loop},
            {"TilingFactor", component.m_TilingFactor},
            {"Flip", component.m_Flip},
            {"Color", {component.m_Color.r, component.m_Color.g, component.m_Color.b, component.m_Color.a}}
        };
    }

    void from_json(const json& j, SpriteAnimatorComponent& component)
    {
        std::string spritesheetpath = j.at("SpriteSheetPath").get<std::string>();

        component.m_SpriteSheet.LoadFromFile(spritesheetpath); // Assuming asset manager handles textures
        component.m_FrameCount = j.at("FrameCount").get<int>();
        component.m_FrameTime = j.at("FrameTime").get<float>();
        component.m_CurrentTime = j.at("CurrentTime").get<float>();
        component.m_CurrentFrame = j.at("CurrentFrame").get<int>();
        component.m_IsPlaying = j.at("IsPlaying").get<bool>();
        component.m_Loop = j.at("Loop").get<bool>();
        component.m_TilingFactor = j.at("TilingFactor").get<float>();
        component.m_Flip = j.at("Flip").get<bool>();
        auto color = j.at("Color").get<std::vector<float>>();
        component.m_Color = { color[0], color[1], color[2], color[3] };
    }

	////////////////////////////////////////////////////////////////////////////////////

	bool SceneSerializer::SaveScene_Text(Scene* scene, const std::filesystem::path& path)
	{
        json sceneJson;

        auto view = scene->m_Registry.view<TagComponent, TransformComponent>();
        for (auto entity : view) {
            json entityJson;
            const auto& tag = scene->m_Registry.get<TagComponent>(entity);
            const auto& transform = scene->m_Registry.get<TransformComponent>(entity);

            entityJson["TagComponent"] = tag;
            entityJson["TransformComponent"] = transform;

            if (scene->m_Registry.any_of<PointLightComponent>(entity))
            {
                entityJson["PointLightComponent"] = scene->m_Registry.get<PointLightComponent>(entity);
            }
            if (scene->m_Registry.any_of<DirectionalLightComponent>(entity))
            {
                entityJson["DirectionalLightComponent"] = scene->m_Registry.get<DirectionalLightComponent>(entity);
            }
            if (scene->m_Registry.any_of<SpriteComponent>(entity))
            {
                entityJson["SpriteComponent"] = scene->m_Registry.get<SpriteComponent>(entity);
            }
            if (scene->m_Registry.any_of<StaticMeshComponent>(entity))
            {
                entityJson["StaticMeshComponent"] = scene->m_Registry.get<StaticMeshComponent>(entity);
            }
            if (scene->m_Registry.any_of<SpriteAnimatorComponent>(entity))
            {
                entityJson["SpriteAnimatorComponent"] = scene->m_Registry.get<SpriteAnimatorComponent>(entity);
            }

            sceneJson["Entities"].push_back(entityJson);
        }

        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << sceneJson.dump(4); // Indent by 4 spaces
        return true;
	}

	bool SceneSerializer::LoadScene_Text(Scene* scene, const std::filesystem::path& path)
	{
        // Read the file
        std::ifstream file(path);
        if (!file.is_open()) return false;

        json sceneJson;
        file >> sceneJson;

        // Clear existing scene
        scene->m_Registry.clear();

        // Iterate through all entities in the JSON
        for (const auto& entityJson : sceneJson["Entities"])
        {
            // Create new entity
            auto entity = scene->m_Registry.create();

            // Load required components (Tag and Transform are mandatory)
            scene->m_Registry.emplace<TagComponent>(entity,
                entityJson["TagComponent"].get<TagComponent>());
            scene->m_Registry.emplace<TransformComponent>(entity,
                entityJson["TransformComponent"].get<TransformComponent>());

            // Load optional components
            if (entityJson.contains("PointLightComponent"))
            {
                scene->m_Registry.emplace<PointLightComponent>(entity,
                    entityJson["PointLightComponent"].get<PointLightComponent>());
            }

            if (entityJson.contains("DirectionalLightComponent"))
            {
                scene->m_Registry.emplace<DirectionalLightComponent>(entity,
                    entityJson["DirectionalLightComponent"].get<DirectionalLightComponent>());
            }

            if (entityJson.contains("SpriteComponent"))
            {
                scene->m_Registry.emplace<SpriteComponent>(entity,
                    entityJson["SpriteComponent"].get<SpriteComponent>());
            }

            if (entityJson.contains("StaticMeshComponent"))
            {
                scene->m_Registry.emplace<StaticMeshComponent>(entity,
                    entityJson["StaticMeshComponent"].get<StaticMeshComponent>());
            }

            if (entityJson.contains("SpriteAnimatorComponent"))
            {
                scene->m_Registry.emplace<SpriteAnimatorComponent>(entity,
                    entityJson["SpriteAnimatorComponent"].get<SpriteAnimatorComponent>());
            }
        }

        return true;
	}
}
