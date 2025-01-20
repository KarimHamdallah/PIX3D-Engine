#pragma once
#include "Scene.h"
#include <Utils/json.hpp>
#include <fstream>

using json = nlohmann::json;

namespace PIX3D
{
    class SceneSerializer
    {
    public:
        SceneSerializer(Scene* scene) : m_Scene(scene) {}

        void Serialize(const std::string& filepath);
        bool Deserialize(const std::string& filepath);

    private:
        void SerializeEntity(entt::entity entity, json& outJson);
        void DeserializeEntity(const json& entityJson);
        void SerializeReferencedAssets(const std::string& filepath);
        void LoadReferencedAssets(const std::string& filepath);

        Scene* m_Scene;
        std::unordered_set<uint64_t> m_ReferencedAssets;  // Track assets in use
    };
}
