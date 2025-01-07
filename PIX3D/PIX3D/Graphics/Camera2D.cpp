#include "Camera2D.h"
#include <Engine/Engine.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace PIX3D
{
	void Camera2D::Init()
	{
        auto specs = PIX3D::Engine::GetApplicationSpecs();
        m_ProjectionMatrix = glm::ortho(
            0.0f,                   // Left
            static_cast<float>(specs.Width),  // Right
            static_cast<float>(specs.Height), // Bottom (flipped)
            0.0f,                   // Top
            -1.0f, 1.0f             // Near and far planes
        );
	}

    glm::mat4 Camera2D::GetViewMatrix() const
    {
        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), glm::vec3(m_Position.x, m_Position.y, 0.0f)) *
            glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotaion), glm::vec3(0.0f, 0.0f, 1.0f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
        
        return glm::inverse(model);
    }
}

