#include "GLDirShadowpass.h"
#include <Core/Core.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <random>

#include <glad/glad.h>
#include <Engine/Engine.hpp>

namespace PIX3D
{
	namespace GL
	{
        void GLDirShadowpass::Init(uint32_t res)
        {
            m_DepthResolution = res;

            m_SimpleDepthShader.LoadFromFile(
                "../PIX3D/res/gl shaders/shadowmapping_depth.vert",
                "../PIX3D/res/gl shaders/shadowmapping_depth.frag",
                "../PIX3D/res/gl shaders/shadowmapping_depth.geom");

            float CameraNearPlane = 0.1f;
            float CameraFarPlane = 1000.0f;

            m_ShadowCascadeLevels =
            {
                CameraFarPlane / 50.0f,
                CameraFarPlane / 25.0f,
                CameraFarPlane / 10.0f,
                CameraFarPlane / 2.0f
            };

            // frame buffer
            glGenFramebuffers(1, &m_Framebuffer);

            // depth textures -- 4
            glGenTextures(1, &m_LightDepthMaps);
            glBindTexture(GL_TEXTURE_2D_ARRAY, m_LightDepthMaps);
            glTexImage3D(
                GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, res, res, int(m_ShadowCascadeLevels.size()) + 1,
                0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

            glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_LightDepthMaps, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE)
                PIX_ASSERT_MSG(false, "Framebuffer is not complete!");

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // light space matrices uniform buffer
            glGenBuffers(1, &m_LightSpaceMatricesUniformbuffer);
            glBindBuffer(GL_UNIFORM_BUFFER, m_LightSpaceMatricesUniformbuffer);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 16, nullptr, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, 10, m_LightSpaceMatricesUniformbuffer);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        void GLDirShadowpass::BeginRender(Camera3D& cam, const glm::vec3& lightdir)
        {
            float CameraNearPlane = cam.GetNearPlane();
            float CameraFarPlane = cam.GetFarPlane();

            m_ShadowCascadeLevels =
            {
                CameraFarPlane / 50.0f,
                CameraFarPlane / 25.0f,
                CameraFarPlane / 10.0f,
                CameraFarPlane / 2.0f
            };

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // 0. UBO setup
            std::vector<glm::mat4> lightMatrices = GetLightSpaceMatrices(cam, lightdir);

            glBindBuffer(GL_UNIFORM_BUFFER, m_LightSpaceMatricesUniformbuffer);
            for (size_t i = 0; i < lightMatrices.size(); ++i)
            {
                glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4), sizeof(glm::mat4), &lightMatrices[i]);
            }
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            //m_LightSpaceMatricesUniformBlock.Update({ lightMatrices.data(), (uint32_t)(lightMatrices.size() * sizeof(glm::mat4)) });

            // 1. render depth of scene to texture (from light's perspective)
            // --------------------------------------------------------------
            //lightProjection = glm::perspective(glm::radians(45.0f), (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT, near_plane, far_plane); // note that if you use a perspective projection matrix you'll have to change the light position as the current light position isn't enough to reflect the whole scene
            // render scene from light's point of view
            glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

            glViewport(0, 0, m_DepthResolution, m_DepthResolution);
            glClear(GL_DEPTH_BUFFER_BIT);
            glCullFace(GL_FRONT);  // peter panning
        }

        void GLDirShadowpass::EndRender()
        {
            glCullFace(GL_BACK);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // reset viewport
            auto specs = Engine::GetApplicationSpecs();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

		GLDirShadowpass::~GLDirShadowpass()
		{
		}

        std::vector<glm::vec4> GLDirShadowpass::GetFrustumCornersWorldSpace(const glm::mat4& projview)
        {
            const auto inv = glm::inverse(projview);

            std::vector<glm::vec4> frustumCorners;
            for (unsigned int x = 0; x < 2; ++x)
            {
                for (unsigned int y = 0; y < 2; ++y)
                {
                    for (unsigned int z = 0; z < 2; ++z)
                    {
                        const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                        frustumCorners.push_back(pt / pt.w);
                    }
                }
            }

            return frustumCorners;
        }


        std::vector<glm::vec4> GLDirShadowpass::GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
        {
            return GetFrustumCornersWorldSpace(proj * view);
        }

        glm::mat4 GLDirShadowpass::GetLightSpaceMatrix(const float nearPlane, const float farPlane, Camera3D& cam, const glm::vec3& lightdir)
        {
            auto specs = Engine::GetApplicationSpecs();

            const auto proj = glm::perspective(
                glm::radians(45.0f), (float)specs.Width / (float)specs.Height, nearPlane,
                farPlane);
            const auto corners = GetFrustumCornersWorldSpace(proj, cam.GetViewMatrix());

            glm::vec3 center = glm::vec3(0, 0, 0);
            for (const auto& v : corners)
            {
                center += glm::vec3(v);
            }
            center /= corners.size();

            const auto lightView = glm::lookAt(center + lightdir, center, glm::vec3(0.0f, 1.0f, 0.0f));

            float minX = std::numeric_limits<float>::max();
            float maxX = std::numeric_limits<float>::lowest();
            float minY = std::numeric_limits<float>::max();
            float maxY = std::numeric_limits<float>::lowest();
            float minZ = std::numeric_limits<float>::max();
            float maxZ = std::numeric_limits<float>::lowest();
            for (const auto& v : corners)
            {
                const auto trf = lightView * v;
                minX = std::min(minX, trf.x);
                maxX = std::max(maxX, trf.x);
                minY = std::min(minY, trf.y);
                maxY = std::max(maxY, trf.y);
                minZ = std::min(minZ, trf.z);
                maxZ = std::max(maxZ, trf.z);
            }

            // Tune this parameter according to the scene
            constexpr float zMult = 10.0f;
            if (minZ < 0)
            {
                minZ *= zMult;
            }
            else
            {
                minZ /= zMult;
            }
            if (maxZ < 0)
            {
                maxZ /= zMult;
            }
            else
            {
                maxZ *= zMult;
            }

            const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
            return lightProjection * lightView;
        }


        std::vector<glm::mat4> GLDirShadowpass::GetLightSpaceMatrices(Camera3D& cam, const glm::vec3& lightdir)
        {
            std::vector<glm::mat4> ret;
            for (size_t i = 0; i < m_ShadowCascadeLevels.size() + 1; ++i)
            {
                if (i == 0)
                {
                    ret.push_back(GetLightSpaceMatrix(cam.GetNearPlane(), m_ShadowCascadeLevels[i], cam, lightdir));
                }
                else if (i < m_ShadowCascadeLevels.size())
                {
                    ret.push_back(GetLightSpaceMatrix(m_ShadowCascadeLevels[i - 1], m_ShadowCascadeLevels[i], cam, lightdir));
                }
                else
                {
                    ret.push_back(GetLightSpaceMatrix(m_ShadowCascadeLevels[i - 1], cam.GetFarPlane(), cam, lightdir));
                }
            }
            return ret;
        }

	}
}
