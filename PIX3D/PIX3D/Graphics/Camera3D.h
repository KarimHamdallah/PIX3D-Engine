#pragma once
#include <glm/glm.hpp>

namespace PIX3D
{
	struct CameraData
	{
		glm::vec3 Position = { 0.0f, 0.0f, 3.0f };

		float Fov = 45.0f;
		float NearPlane = 0.1f;
		float FarPlane = 1000.0f;
		float Speed = 10.0f;
		float MovementSmoothness = 5.0f;

		float ScrollSpeedScaler = 1.0f;
		float ScrollSmoothness = 10.0f;

		float TurnSpeed = 0.1f;
		float TurnSmoothness = 10.0f;

		float Pitch = 0.0f;
		float Yaw = -90.0f;
	};

	class Camera3D
	{
	public:
		Camera3D() = default;
		~Camera3D() {}

		void Init(const glm::vec3& position);

		void Update(float dt);

		glm::mat4 GetProjectionMatrix() const;
		glm::mat4 GetViewMatrix() const;

		glm::vec3 GetPosition() const { return m_CameraData.Position; }

		CameraData GetCameraData() { return m_CameraData; }

		float GetNearPlane() { return m_CameraData.NearPlane; }
		float GetFarPlane() { return m_CameraData.FarPlane; }

	private:
		CameraData m_CameraData;

		glm::vec3 m_CameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 m_CameraRight = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 m_CameraUp = glm::vec3(0.0f, 0.0f, 0.0f);

	private:
		glm::vec3 m_TargetPosition;
		glm::vec3 m_CameraForwardDirection;

		float m_TargetPitch = 0.0f;
		float m_TargetYaw = -90.0f;
		float m_TargetFov = 45.0f;
	};
}
