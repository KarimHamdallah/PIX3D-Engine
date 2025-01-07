#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace PIX3D
{

	struct Transform
	{
		operator const glm::mat4() const
		{
			return GetTransformMatrix();
		}

		glm::mat4 GetTransformMatrix() const
		{
			glm::mat4 Transform = glm::mat4(1.0f);
			Transform = glm::translate(Transform, Position);
			Transform = glm::rotate(Transform, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			Transform = glm::rotate(Transform, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			Transform = glm::rotate(Transform, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			Transform = glm::scale(Transform, Scale);
			return Transform;

			/*
			return glm::translate(glm::mat4(1.0f), m_Position) *
				glm::toMat4(glm::quat(glm::vec3(glm::radians(m_Rotation.x), glm::radians(m_Rotation.y), glm::radians(m_Rotation.z)))) *
				glm::scale(glm::mat4(1.0f), m_Scale);
			*/

		}

		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };
	};
}
