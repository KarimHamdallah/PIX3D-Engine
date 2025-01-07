#include "GLPostProcessingpass.h"
#include "GLStaticMeshFactory.h"
#include "GLCommands.h"
#include <Core/Core.h>
#include <glad/glad.h>
#include <Engine/Engine.hpp>

void PIX3D::GL::GLPostProcessingpass::Init(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;

    /*
    // create the framebuffer
    glGenFramebuffers(1, &m_Framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

    // create color texture
    glGenTextures(1, &m_ColorAttachment);
    glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);

    glBindTexture(GL_TEXTURE_2D, 0);

    // create depth/stencil buffer
    // we use renderbuffer which is similar to textures except you can't sample it
    // that is okay because we never actually need to read the values in depth/stencil buffer
    // its all done by hardware depth/stencil tests
    glGenRenderbuffers(1, &m_DepthStencilRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_DepthStencilRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // attach the renderbuffer to the framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthStencilRenderbuffer);

    unsigned int colorAttachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, colorAttachments);

    // make sure the framebuffer was created successfully
    PIX_ASSERT_MSG(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Error initializing framebuffer: framebuffer not complete!");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    */
    m_PostProcessingShader.LoadFromFile("../PIX3D/res/gl shaders/post.vert", "../PIX3D/res/gl shaders/post.frag");
}

void PIX3D::GL::GLPostProcessingpass::Resize(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;

    // resize color textures
    glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // resize renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, m_DepthStencilRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void PIX3D::GL::GLPostProcessingpass::Render(uint32_t main_renderpass_color_attachment, uint32_t bloom_renderpass_output_color_attachment)
{
    m_PostProcessingShader.Bind();
    m_PostProcessingShader.SetBool("u_BloomEnabled", m_BloomEnabled);
    m_PostProcessingShader.SetFloat("u_BloomIntensity", m_BloomIntensity);
    m_PostProcessingShader.SetBool("u_TonemappingEnabled", m_TonemappingEnabled);
    m_PostProcessingShader.SetFloat("u_GammaCorrectionFactor", m_GammaCorrectionFactor);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, main_renderpass_color_attachment);
    m_PostProcessingShader.SetInt("u_ColorTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloom_renderpass_output_color_attachment);
    m_PostProcessingShader.SetInt("u_BloomTexture", 1);


    glDisable(GL_DEPTH_TEST);

    GLStaticMeshData QuadMesh = GLStaticMeshGenerator::GenerateQuad();
    QuadMesh.VertexArray.Bind();
    GLCommands::DrawArrays(Primitive::TRIANGLES, QuadMesh.VerticesCount);

    glEnable(GL_DEPTH_TEST);
}
