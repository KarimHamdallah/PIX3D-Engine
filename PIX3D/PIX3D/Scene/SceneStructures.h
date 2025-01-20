#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace PIX3D
{
    struct TransformData
    {
        glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

        glm::mat4 GetTransformMatrix() const
        {
            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, Position);
            transform = glm::rotate(transform, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            transform = glm::rotate(transform, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            transform = glm::rotate(transform, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            transform = glm::scale(transform, Scale);
            return transform;
        }

        operator glm::mat4() const { return GetTransformMatrix(); }
    };

    struct SpriteData
    {
        PIX3D::UUID TextureUUID = 0;
        glm::vec4 Color = { 1.0, 1.0, 1.0, 1.0 };
        float TilingFactor = 1.0f;
    };
}
