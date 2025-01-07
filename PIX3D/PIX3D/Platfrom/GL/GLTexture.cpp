#include "GLTexture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <Utils/stb_image.h>
#include <Core/Core.h>
#include <glad/glad.h>

PIX3D::GL::GLTexture::~GLTexture()
{
}

void PIX3D::GL::GLTexture::LoadFromData(uint8_t* data, uint32_t width, uint32_t height)
{
    glCreateTextures(GL_TEXTURE_2D, 1, &m_Handle);

    glTextureParameteri(m_Handle, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_Handle, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(m_Handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_Handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTextureStorage2D(m_Handle, 1, GL_RGBA8, width, height);
    glTextureSubImage2D(m_Handle, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

    //m_TextureID = glGetTextureHandleARB(m_Handle); // Generate ID For Texture In Vram
    //glMakeTextureHandleResidentARB(m_TextureID); // Make It Visible To Shaders

    glBindTexture(GL_TEXTURE_2D, 0);
}

void PIX3D::GL::GLTexture::LoadFromFile(const std::string& path, bool srgb)
{
    m_Path = path;

    int w, h, bpp;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &bpp, 0);

    PIX_ASSERT_MSG(data, "Failed To Load Texture!");


    GLenum format;

    switch (bpp) {
    case 1:
        format = GL_RED;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 4:
        format = GL_RGBA;
        break;
    }

    GLenum internalFormat = format;

    // account for sRGB textures here
    //
    // diffuse textures are in sRGB space (non-linear)
    // metallic/roughness/normals are usually in linear
    // AO depends
    if (srgb)
    {
        if (internalFormat == GL_RGB)
        {
            internalFormat = GL_SRGB;
        }
        else if (internalFormat == GL_RGBA)
        {
            internalFormat = GL_SRGB_ALPHA;
        }
    }

    glGenTextures(1, &m_Handle);
    glBindTexture(GL_TEXTURE_2D, m_Handle);

    // generate the texture
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // texture wrapping/filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // image is resized using bilinear filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // image is enlarged using bilinear filtering

    //m_TextureID = glGetTextureHandleARB(m_Handle); // Generate ID For Texture In Vram
    //glMakeTextureHandleResidentARB(m_TextureID); // Make It Visible To Shaders

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
}

void PIX3D::GL::GLTexture::Bind(uint32_t slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_Handle);
}

void PIX3D::GL::GLTexture::Destroy()
{
    glMakeTextureHandleNonResidentARB(m_TextureID);
    glDeleteTextures(1, &m_Handle);
}
