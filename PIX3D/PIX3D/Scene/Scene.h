#pragma once
#include <Core/Core.h>
#include "Components.h"
#include <Graphics/Camera3D.h>
#include <Graphics/Transform.h>
#include <Graphics/Sprite.h>
#include <Utils/entt.hpp>
#include <Platfrom/GL/IBL/IBLCubemapsGenerator.h>

#define MAX_POINT_LIGHTS 100

namespace PIX3D
{

	struct PointLightShaderBuffer
	{
		glm::vec4 LightPosition;
		glm::vec4 LightColor;
		float Intensity;
		float Raduis;
		float Falloff;
	};

	class Scene
	{
	public:
		Scene(const std::string& name);
		uint32_t AddGameObject(const std::string& name, const Transform& transform);
		uint32_t AddStaticMesh(const std::string& name, const Transform& transform, const StaticMesh& static_mesh);
		uint32_t AddSprite(const std::string& name, const Transform& transform, const Sprite& sprite);
		uint32_t AddPointLight(const std::string& name, const Transform& transform, const glm::vec4& color);
		uint32_t AddDirectionalLight(const std::string& name, const Transform& transform, const glm::vec4& color);
		uint32_t AddSpriteAnimation(const std::string& name, const Transform& transform, const GL::GLTexture& spriteSheet, int frameCount, float frameTime);

		void OnStart();
		void OnUpdate(float dt);
		void OnRender();

	public:
		std::string m_Name;
		entt::registry m_Registry;

		// Camera3D
		Camera3D m_Cam3D;

		// IBL Lightning
		GL::GLHdriCubemap m_Cubemap;
		Transform m_CubemapTransform;
		GL::IBLMaps m_IBLMaps;
		int m_EnvironmentMapSize = 1024;
		bool m_UseSkybox = true;
		glm::vec4 m_BackgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		// PointLights Buffer
		std::vector<PointLightShaderBuffer> m_PointLightsShaderData;
		GL::GLStorageBuffer m_PointLightsBuffer;

		int m_PointLightsCount = 0;

		DirectionalLightComponent m_DirLight;
		bool m_HasDirectionalLight = false;
	};
}
