#include "Camera3D.h"
#include <Core/Input.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Engine/Engine.hpp>
#include <iostream>

namespace
{
	glm::vec3 LerpVec3(const glm::vec3& start, const glm::vec3& end, float t)
	{
		t = glm::clamp(t, 0.0f, 1.0f);
		return start + t * (end - start);
	}

	float Lerpf(float start, float end, float t)
	{
		t = glm::clamp(t, 0.0f, 1.0f);
		return start + t * (end - start);
	}
}

namespace PIX3D
{
	void Camera3D::Init(const glm::vec3& position)
	{
		m_CameraData.Position = position;
		m_TargetPosition = position;

		glm::vec3 CameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 CameraDirection = glm::normalize(m_CameraData.Position - CameraTarget);

		glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
		m_CameraRight = glm::normalize(glm::cross(Up, CameraDirection));

		m_CameraUp = glm::cross(CameraDirection, m_CameraRight);
	}

	void Camera3D::Update(float dt)
	{
		
		if (Input::IsKeyPressed(KeyCode::LeftShift))
		{
			auto MouseOffset = Input::GetMouseOffset();
			
			m_TargetYaw += MouseOffset.x * m_CameraData.TurnSpeed;
			m_TargetPitch += MouseOffset.y * m_CameraData.TurnSpeed;

			if (m_TargetPitch > 89.0f)
				m_TargetPitch = 89.0f;
			if (m_TargetPitch < -89.0f)
				m_TargetPitch = -89.0f;

			m_CameraData.Yaw = Lerpf(m_CameraData.Yaw, m_TargetYaw, m_CameraData.TurnSmoothness * dt);
			m_CameraData.Pitch = Lerpf(m_CameraData.Pitch, m_TargetPitch, m_CameraData.TurnSmoothness * dt);
		}

		auto mousescroll = Input::GetMouseScroll();
		m_TargetFov -= (float)mousescroll.y * m_CameraData.ScrollSpeedScaler;
		m_TargetFov = glm::clamp(m_TargetFov, 1.0f, 45.0f);

		m_CameraData.Fov = Lerpf(m_CameraData.Fov, m_TargetFov, m_CameraData.ScrollSmoothness * dt);
	

		m_CameraForwardDirection.x = cos(glm::radians(m_CameraData.Yaw)) * cos(glm::radians(m_CameraData.Pitch));
		m_CameraForwardDirection.y = sin(glm::radians(m_CameraData.Pitch));
		m_CameraForwardDirection.z = sin(glm::radians(m_CameraData.Yaw)) * cos(glm::radians(m_CameraData.Pitch));

		m_CameraForwardDirection = glm::normalize(m_CameraForwardDirection);


		glm::vec3 _up = glm::vec3(0.0f, 1.0f, 0.0f);
		m_CameraRight = glm::normalize(glm::cross(_up, m_CameraForwardDirection));

		m_CameraUp = glm::cross(m_CameraForwardDirection, m_CameraRight);

		if (Input::IsKeyPressed(PIX3D::KeyCode::A))
			m_TargetPosition += m_CameraRight * m_CameraData.Speed * dt;
		else if (Input::IsKeyPressed(PIX3D::KeyCode::D))
			m_TargetPosition -= m_CameraRight * m_CameraData.Speed * dt;

		if (Input::IsKeyPressed(PIX3D::KeyCode::E))
			m_TargetPosition += m_CameraUp * m_CameraData.Speed * dt;
		else if (Input::IsKeyPressed(PIX3D::KeyCode::Q))
			m_TargetPosition -= m_CameraUp * m_CameraData.Speed * dt;

		if (Input::IsKeyPressed(PIX3D::KeyCode::W))
			m_TargetPosition += m_CameraForwardDirection * m_CameraData.Speed * dt;
		else if (Input::IsKeyPressed(PIX3D::KeyCode::S))
			m_TargetPosition -= m_CameraForwardDirection * m_CameraData.Speed * dt;

		m_CameraData.Position = LerpVec3(m_CameraData.Position, m_TargetPosition, dt * m_CameraData.MovementSmoothness);
	}

	glm::mat4 Camera3D::GetViewMatrix() const
	{
		return glm::lookAt(m_CameraData.Position, m_CameraData.Position + m_CameraForwardDirection, m_CameraUp);
	}

	glm::mat4 Camera3D::GetProjectionMatrix() const
	{
		auto AppSpecs = PIX3D::Engine::GetApplicationSpecs();
		float AspectRatio = (float)AppSpecs.Width / AppSpecs.Height;

		return glm::perspective(glm::radians(m_CameraData.Fov), AspectRatio, m_CameraData.NearPlane, m_CameraData.FarPlane);
	}
}
