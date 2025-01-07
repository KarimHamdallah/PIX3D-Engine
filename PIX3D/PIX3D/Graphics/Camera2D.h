#pragma once
#include <glm/glm.hpp>

namespace PIX3D
{

	class Camera2D
	{
	public:
		Camera2D() = default;
		~Camera2D() {}

		void Init();

		inline glm::mat4 GetProjectionMatrix() const { return m_ProjectionMatrix; }

		glm::mat4 GetViewMatrix() const;

		glm::vec2 GetPosition() { return m_Position; }
		float GetRotaion() { return m_Rotaion; }

		void SetPosition(const glm::vec2& pos) { m_Position = pos; }
		void SetRotaion(float rot) { m_Rotaion = rot; }

	private:
		glm::mat4 m_ProjectionMatrix;

		glm::vec2 m_Position = { 0.0f, 0.0f };
		float m_Rotaion = 0.0f;
	};
}
