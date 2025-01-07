#include "IBLCubemapsGenerator.h"
#include <glm/gtc/matrix_transform.hpp>
#include "../GLCommands.h"
#include "../GLStaticMeshFactory.h"
#include <glad/glad.h>
#include <Engine/Engine.hpp>

# define M_PI           3.14159265358979323846  /* pi */

namespace PIX3D
{
	namespace GL
	{
		namespace
		{
            glm::vec3 importanceSampleGGX(glm::vec2 Xi, float roughness, glm::vec3 N)
            {
                float a = roughness * roughness;

                // Sample in spherical coordinates
                float phi = 2.0f * M_PI * Xi.x;
                float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
                float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

                // Convert to cartesian coordinates
                glm::vec3 H;
                H.x = sinTheta * cos(phi);
                H.y = sinTheta * sin(phi);
                H.z = cosTheta;

                // Transform from tangent to world space
                glm::vec3 up = abs(N.z) < 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
                glm::vec3 tangentX = glm::normalize(glm::cross(up, N));
                glm::vec3 tangentY = glm::cross(N, tangentX);

                return glm::normalize(tangentX * H.x + tangentY * H.y + N * H.z);
            }
		}

        void IBLCubemapsGenerator::Init()
        {
            s_DiffuseIrradianceShader.LoadFromFile("../PIX3D/res/gl shaders/diffuse_irradiance.vert", "../PIX3D/res/gl shaders/diffuse_irradiance.frag");
            s_SpecularIBLShader.LoadFromFile("../PIX3D/res/gl shaders/ibl_specular_map.vert", "../PIX3D/res/gl shaders/ibl_specular_map.frag");
            s_BrdfLUTShader.LoadFromFile("../PIX3D/res/gl shaders/brdf_lut.vert", "../PIX3D/res/gl shaders/brdf_lut.frag");

            s_BrdfLUTTextureHandle = GenerateBRDFLUTTexture();
        }

        void IBLCubemapsGenerator::Destroy()
        {
            s_DiffuseIrradianceShader.Destroy();
            s_SpecularIBLShader.Destroy();
            s_BrdfLUTShader.Destroy();
        }

        IBLMaps IBLCubemapsGenerator::GenerateIBLMaps(uint32_t envcubemap, int irradiancesize, int prefiltersize)
        {
            // Create and setup irradiance cubemap
            uint32_t irradianceMap, prefilterMap = 0;

            glGenTextures(1, &irradianceMap);
            glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
            for (unsigned int i = 0; i < 6; ++i)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                    irradiancesize, irradiancesize, 0, GL_RGB, GL_FLOAT, nullptr);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Create and setup prefilter cubemap
            glGenTextures(1, &prefilterMap);
            glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
            for (unsigned int i = 0; i < 6; ++i)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                    prefiltersize, prefiltersize, 0, GL_RGB, GL_FLOAT, nullptr);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

            // Setup framebuffer for convolution
            GLuint captureFBO, captureRBO;
            glGenFramebuffers(1, &captureFBO);
            glGenRenderbuffers(1, &captureRBO);
            glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);

            // Setup capture matrices
            glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
            glm::mat4 captureViews[] = 
            {
                glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
                glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
                glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
                glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
                glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
                glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
            };

            // Generate irradiance 

            glViewport(0, 0, irradiancesize, irradiancesize);
            glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

            s_DiffuseIrradianceShader.Bind();
            s_DiffuseIrradianceShader.SetInt("environmentCubemap", 0);
            s_DiffuseIrradianceShader.SetMat4("projection", captureProjection);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, envcubemap);

            for (unsigned int i = 0; i < 6; ++i)
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                s_DiffuseIrradianceShader.SetMat4("view", captureViews[i]);
                s_DiffuseIrradianceShader.SetMat4("model", glm::mat4(1.0f));
                
                {
                    GLStaticMeshData CubeMesh = GLStaticMeshGenerator::GenerateCube();
                    CubeMesh.VertexArray.Bind();
                    GLCommands::DrawArrays(Primitive::TRIANGLES, CubeMesh.VerticesCount);
                }
            }

            // Generate prefilter map with roughness
            s_SpecularIBLShader.Bind();
            s_SpecularIBLShader.SetInt("environmentCubemap", 0);
            s_SpecularIBLShader.SetMat4("projection", captureProjection);

            const unsigned int maxMipLevels = 5;
            for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
            {
                unsigned int mipSize = prefiltersize * std::pow(0.5, mip);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                    GL_RENDERBUFFER, captureRBO);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipSize, mipSize);
                glViewport(0, 0, mipSize, mipSize);

                float roughness = (float)mip / (float)(maxMipLevels - 1);
                
                s_SpecularIBLShader.SetFloat("roughness", roughness);

                for (unsigned int i = 0; i < 6; ++i)
                {
                    s_SpecularIBLShader.SetMat4("view", captureViews[i]);
                    s_SpecularIBLShader.SetMat4("model", glm::mat4(1.0f));

                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    
                    {
                        GLStaticMeshData CubeMesh = GLStaticMeshGenerator::GenerateCube();
                        CubeMesh.VertexArray.Bind();
                        GLCommands::DrawArrays(Primitive::TRIANGLES, CubeMesh.VerticesCount);
                    }
                }
            }

            IBLMaps maps;
            maps.IrradianceMap.Load(irradianceMap);
            maps.PrefilterMap.Load(prefilterMap);
            maps.IrradianceSize = irradiancesize;
            maps.PrefilterSize = prefiltersize;


            // Cleanup
            glDeleteFramebuffers(1, &captureFBO);
            glDeleteRenderbuffers(1, &captureRBO);

            {
                auto specs = Engine::GetApplicationSpecs();
                GLCommands::SetViewPort(specs.Width, specs.Height);
            }

            return maps;
        }

        uint32_t IBLCubemapsGenerator::GenerateBRDFLUTTexture(unsigned int resolution)
        {
            uint32_t framebufferId, depthRenderbufferId = 0;

            glGenFramebuffers(1, &framebufferId);
            glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);

            // depth buffer
            glGenRenderbuffers(1, &depthRenderbufferId);
            glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbufferId);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, resolution, resolution);

            // attach the depth buffer
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbufferId);

            uint32_t colorTextureId = 0;

            // color texture
            glGenTextures(1, &colorTextureId);
            glBindTexture(GL_TEXTURE_2D, colorTextureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, resolution, resolution, 0, GL_RGBA, GL_FLOAT, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTextureId, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);

            // Render
            s_BrdfLUTShader.Bind();
            
            glViewport(0, 0, resolution, resolution);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            GLStaticMeshData QuadMesh = GLStaticMeshGenerator::GenerateQuad();
            QuadMesh.VertexArray.Bind();
            GLCommands::DrawArrays(Primitive::TRIANGLES, QuadMesh.VerticesCount);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);


            // Cleanup
            glDeleteFramebuffers(1, &framebufferId);
            glDeleteRenderbuffers(1, &depthRenderbufferId);

            {
                auto specs = Engine::GetApplicationSpecs();
                GLCommands::SetViewPort(specs.Width, specs.Height);
            }

            return colorTextureId;
        }
	}
}
