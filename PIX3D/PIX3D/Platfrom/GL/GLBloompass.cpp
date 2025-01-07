#include "GLBloompass.h"
#include "GLCommands.h"
#include "GLStaticMeshFactory.h"
#include <glad/glad.h>
#include <Engine/Engine.hpp>

namespace PIX3D
{
	namespace GL
	{

		void GLBloompass::Init(uint32_t width, uint32_t height)
		{
            m_Width = width;
            m_Height = height;

			m_BloomShader.LoadFromFile("../PIX3D/res/gl shaders/bloom.vert", "../PIX3D/res/gl shaders/bloom.frag");

            m_Framebuffers.resize(2);
            m_ColorAttachments.resize(2);

            // First Framebuffer
            {
                // create the framebuffer
                glGenFramebuffers(1, &m_Framebuffers[0]);
                glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffers[0]);

                // create a color texture
                glGenTextures(1, &m_ColorAttachments[0]);
                glBindTexture(GL_TEXTURE_2D, m_ColorAttachments[0]);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glGenerateMipmap(GL_TEXTURE_2D);

                // attach the color texture to the framebuffer
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachments[0], 0);
                glBindTexture(GL_TEXTURE_2D, 0);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }

            // Second Framebuffer
            {
                // create the framebuffer
                glGenFramebuffers(1, &m_Framebuffers[1]);
                glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffers[1]);

                // create a color texture
                glGenTextures(1, &m_ColorAttachments[1]);
                glBindTexture(GL_TEXTURE_2D, m_ColorAttachments[1]);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glGenerateMipmap(GL_TEXTURE_2D);

                // attach the color texture to the framebuffer
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachments[1], 0);
                glBindTexture(GL_TEXTURE_2D, 0);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
		}

        void GLBloompass::Resize(uint32_t width, uint32_t height)
        {
            m_Width = width;
            m_Height = height;

            // resize color textures
            {
                glBindTexture(GL_TEXTURE_2D, m_ColorAttachments[0]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
                glGenerateMipmap(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            {
                glBindTexture(GL_TEXTURE_2D, m_ColorAttachments[1]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
                glGenerateMipmap(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }

        void GLBloompass::Render(uint32_t bloom_brightness_color_attachment)
        {
            // Bloom pass
            glm::vec2 blurDirectionX = glm::vec2(1.0f, 0.0f);
            glm::vec2 blurDirectionY = glm::vec2(0.0f, 1.0f);


            switch (m_BloomDirection)
            {
            case GLBloomDirection::HORIZONTAL:
                blurDirectionY = blurDirectionX;
                break;
            case GLBloomDirection::VERTICAL:
                blurDirectionX = blurDirectionY;
            default:
                break;
            }

            glBindTexture(GL_TEXTURE_2D, bloom_brightness_color_attachment);
            glGenerateMipmap(GL_TEXTURE_2D);

            m_BloomShader.Bind();

            for (auto mipLevel = 0; mipLevel <= 5; mipLevel++)
            {
                SetMipLevels(mipLevel);

                // first iteration we use the bloom buffer from the main render pass
                {
                    int width = m_Width / std::pow(2, mipLevel);
                    int height = m_Height / std::pow(2, mipLevel);

                    glViewport(0, 0, width, height);
                    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffers[0]);
                }
                glBindTexture(GL_TEXTURE_2D, bloom_brightness_color_attachment);
                m_BloomShader.SetInt("u_SampleMipLevel", mipLevel);
                m_BloomShader.SetVec2("u_BlurDirection", blurDirectionX);

                glDisable(GL_DEPTH_TEST);
                GLStaticMeshData QuadMesh = GLStaticMeshGenerator::GenerateQuad();
                QuadMesh.VertexArray.Bind();
                GLCommands::DrawArrays(Primitive::TRIANGLES, QuadMesh.VerticesCount);
                glEnable(GL_DEPTH_TEST);

                unsigned int bloomFramebuffer = 1; // which buffer to use

                for (auto i = 1; i < m_BloomIterations; i++)
                {
                    unsigned int sourceBuffer = bloomFramebuffer == 1 ? 0 : 1;
                    {
                        int width = m_Width / std::pow(2, mipLevel);
                        int height = m_Height / std::pow(2, mipLevel);

                        glViewport(0, 0, width, height);
                        glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffers[bloomFramebuffer]);
                    }
                    auto blurDirection = bloomFramebuffer == 1 ? blurDirectionY : blurDirectionX;
                    m_BloomShader.SetVec2("u_BlurDirection", blurDirection);
                    glBindTexture(GL_TEXTURE_2D, m_ColorAttachments[sourceBuffer]);
                    glDisable(GL_DEPTH_TEST);
                    GLStaticMeshData QuadMesh = GLStaticMeshGenerator::GenerateQuad();
                    QuadMesh.VertexArray.Bind();
                    GLCommands::DrawArrays(Primitive::TRIANGLES, QuadMesh.VerticesCount);
                    glEnable(GL_DEPTH_TEST);
                    bloomFramebuffer = sourceBuffer;
                }

                m_BloomFramebufferResult = bloomFramebuffer;
            }

            {
                auto specs = Engine::GetApplicationSpecs();
                GLCommands::SetViewPort(specs.Width, specs.Height);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0); // switch back to default fb
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        void GLBloompass::SetMipLevels(int miplevel)
        {
            {
                glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffers[0]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachments[0], miplevel);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            {
                glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffers[1]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachments[1], miplevel);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
        }

        void GLBloompass::Destroy()
        {
            m_BloomShader.Destroy();
            for (size_t i = 0; i < m_Framebuffers.size(); i++)
            {
                glDeleteFramebuffers(1, &m_Framebuffers[i]);
                glDeleteTextures(1, &m_ColorAttachments[i]);
            }
        }
	}
}
