#pragma once
#include <string>
#include <filesystem>
#include "Core/UUID.h"
#include <Utils/json.hpp>

using json = nlohmann::json;

namespace PIX3D
{
    enum class AssetType
    {
        None = 0,
        StaticMesh,
        Texture,
        Material
    };

    class Asset
    {
    public:
        Asset() = default;
        Asset(AssetType type) : m_Type(type) {}
        virtual ~Asset() = default;

        const UUID& GetUUID() const { return m_UUID; }
        void SetUUID(uint64_t uuid) { m_UUID = uuid; }
        AssetType GetType() const { return m_Type; }
        const std::filesystem::path& GetPath() const { return m_Path; }

    protected:
        UUID m_UUID;
        AssetType m_Type = AssetType::None;
        std::filesystem::path m_Path;
        friend class AssetManager;
    };
}
