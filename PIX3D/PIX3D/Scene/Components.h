#pragma once
#include <string>
#include <glm/glm.hpp>
#include <Graphics/VulkanStaticMesh.h>
#include <Graphics/Material.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace PIX3D
{
    struct TagComponent
    {
        TagComponent() = default;
        TagComponent(const TagComponent& other) = default;
        TagComponent(const std::string& tag) : m_Tag(tag) {}

        std::string m_Tag;
    };

    struct TransformComponent
    {
        TransformComponent() = default;
        TransformComponent(const TransformComponent& other) = default;
        TransformComponent(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
            : m_Position(position), m_Rotation(rotation), m_Scale(scale) {}

        glm::mat4 GetTransformMatrix() const
        {
            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, m_Position);
            transform = glm::rotate(transform, glm::radians(m_Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            transform = glm::rotate(transform, glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            transform = glm::rotate(transform, glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            transform = glm::scale(transform, m_Scale);
            return transform;
        }

        operator glm::mat4() const { return GetTransformMatrix(); }

        glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
        glm::vec3 m_Rotation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 m_Scale = { 1.0f, 1.0f, 1.0f };
    };


    struct SpriteComponent
    {
        SpriteComponent() = default;
        SpriteComponent(const SpriteComponent& other) = default;
        SpriteComponent(SpriteMaterial* material) : m_Material(material) {}

        ~SpriteComponent()
        {
            if (m_Material)
            {
                m_Material->Destroy();
                delete m_Material;
                m_Material = nullptr;
            }
        }

        SpriteMaterial* m_Material = nullptr;
    };

    struct StaticMeshComponent
    {
        StaticMeshComponent() = default;
        StaticMeshComponent(const StaticMeshComponent& other) = default;
        StaticMeshComponent(const VulkanStaticMesh& mesh) : m_Mesh(mesh) {}

        ~StaticMeshComponent()
        {
            m_Mesh.Destroy();
        }

        VulkanStaticMesh m_Mesh;
    };

    struct PointLightComponent
    {
        PointLightComponent() = default;
        PointLightComponent(const PointLightComponent& other) = default;
        PointLightComponent(const glm::vec4& color) : m_Color(color) {}

        // Aligned for GPU buffer compatibility
        alignas(16) glm::vec4 m_Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        alignas(4) float m_Intensity = 1.0f;
        alignas(4) float m_Radius = 1.0f;
        alignas(4) float m_Falloff = 1.0f;
    };

    struct DirectionalLightComponent
    {
        DirectionalLightComponent() = default;
        DirectionalLightComponent(const DirectionalLightComponent& other) = default;
        DirectionalLightComponent(const glm::vec4& color) : m_Color(color) {}

        // Aligned for GPU buffer compatibility
        alignas(16) glm::vec3 m_Direction = { 0.3f, -1.0f, 0.2f };
        alignas(16) glm::vec4 m_Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        alignas(4) float m_Intensity = 1.0f;
    };

    struct SpriteAnimatorComponent
    {
        SpriteAnimatorComponent() = default;
        SpriteAnimatorComponent(const SpriteAnimatorComponent& other) = default;
        SpriteAnimatorComponent(VK::VulkanTexture* spriteSheet, int frameCount, float frameTime)
            : m_FrameCount(frameCount), m_FrameTime(frameTime)
        {
            // Create sprite material with spritesheet texture
            m_Material = new SpriteMaterial();
            m_Material->Create(spriteSheet);
            m_Material->m_Data->color = m_Color;
            m_Material->m_Data->tiling_factor = m_TilingFactor;
            m_Material->m_Data->flip = m_Flip;
            m_Material->m_Data->uv_scale = { 1.0f / frameCount, 1.0f };
            m_Material->m_Data->uv_offset = { 0.0f, 0.0f };
            m_Material->m_Data->apply_uv_scale_and_offset = 1;
            m_Material->UpdateBuffer();
        }

        ~SpriteAnimatorComponent()
        {
            if (m_Material)
            {
                m_Material->Destroy();
                delete m_Material;
                m_Material = nullptr;
            }
        }

        SpriteMaterial* m_Material = nullptr;
        int m_FrameCount = 1;
        float m_FrameTime = 0.1f;
        float m_CurrentTime = 0.0f;
        int m_CurrentFrame = 0;
        bool m_IsPlaying = false;
        bool m_Loop = true;
        float m_TilingFactor = 1.0f;
        bool m_Flip = true;
        glm::vec4 m_Color = { 1.0f, 1.0f, 1.0f, 1.0f };
    };
}
