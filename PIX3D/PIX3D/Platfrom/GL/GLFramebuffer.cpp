#include "GLFramebuffer.h"
#include <Core/Core.h>
#include <glad/glad.h>

namespace PIX3D
{
	namespace GL
	{
		GLFramebuffer::~GLFramebuffer()
		{
            glDeleteFramebuffers(1, &m_Handle);
            glDeleteTextures(1, &m_ColorAttachmentHandle);
            glDeleteRenderbuffers(1, &m_RenderbufferHandle);
		}

		void GLFramebuffer::Create(const GLFramebufferSpecification& specs)
		{
			m_Specs = specs;

            glGenFramebuffers(1, &m_Handle);
            glBindFramebuffer(GL_FRAMEBUFFER, m_Handle);


            // Create a Color Attachment texture
            glGenTextures(1, &m_ColorAttachmentHandle);
            glBindTexture(GL_TEXTURE_2D, m_ColorAttachmentHandle);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, specs.Width, specs.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachmentHandle, 0);

            // Create multisample depth/stencil Attachment
            GLuint depthStencilBuffer;
            glGenRenderbuffers(1, &depthStencilBuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, depthStencilBuffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, specs.SamplesCount, GL_DEPTH24_STENCIL8, specs.Width, specs.Height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilBuffer);

            // Check framebuffer status
            if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                PIX_ASSERT_MSG(false, "Framebuffer is not complete!");

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

        void GLFramebuffer::Begin()
        {
            glBindFramebuffer(GL_FRAMEBUFFER, m_Handle);
        }

        void GLFramebuffer::End()
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void GLFramebuffer::Resize(uint32_t width, uint32_t height)
        {
            if (width == 0 || height == 0)
                return;

            m_Specs.Width = width;
            m_Specs.Height = height;

            glDeleteFramebuffers(1, &m_Handle);
            glDeleteTextures(1, &m_ColorAttachmentHandle);
            glDeleteRenderbuffers(1, &m_RenderbufferHandle);

            Create(m_Specs);
        }

        std::vector<uint8_t> GLFramebuffer::GetPixels(uint32_t bpp)
        {
            // Create a vector to hold pixel data
            std::vector<unsigned char> pixels(m_Specs.Width * m_Specs.Height * bpp); // 4 bytes per pixel (RGBA)
            
            // Bind the framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, m_Handle);

            // Read the pixels from the framebuffer
            glReadPixels(0, 0, m_Specs.Width, m_Specs.Height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

            // Unbind the framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            return pixels;
        }

        std::vector<uint32_t> GLFramebuffer::GetPixelsUint32()
        {
            // Create a 1D vector to hold the pixel data
            std::vector<uint32_t> pixels(m_Specs.Width * m_Specs.Height);

            // Bind the framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, m_Handle);

            // Read the pixels from the framebuffer (assume RGBA, 4 bytes per pixel)
            glReadPixels(0, 0, m_Specs.Width, m_Specs.Height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

            // Unbind the framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            return pixels;
        }

        void GLFramebuffer::SetPixelsUint32(const std::vector<uint32_t>& pixels)
        {
            // Validate input dimensions
            if (pixels.size() != m_Specs.Width * m_Specs.Height)
            {
                PIX_ASSERT_MSG(false, "Pixel array size does not match framebuffer dimensions.");
            }

            // Bind the framebuffer's texture (assuming color attachment is at GL_COLOR_ATTACHMENT0)
            glBindFramebuffer(GL_FRAMEBUFFER, m_Handle);

            // Upload the pixel data to the framebuffer's texture (glTexSubImage2D updates a portion of the texture)
            glTexSubImage2D(
                GL_TEXTURE_2D,            // Target
                0,                        // Level of detail
                0, 0,                     // Offset (x, y)
                m_Specs.Width, m_Specs.Height, // Dimensions
                GL_RGBA,                  // Format
                GL_UNSIGNED_BYTE,         // Data type
                pixels.data()             // Data
            );

            // Unbind the framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
	}
}
