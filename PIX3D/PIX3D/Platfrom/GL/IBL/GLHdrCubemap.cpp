#include "GLHdrCubemap.h"
#include <Core/Core.h>
#include <glm/glm.hpp>
#include <glad/glad.h>

#include <Utils/stb_image.h>

# define M_PI           3.14159265358979323846  /* pi */

namespace
{
    glm::vec3 getFaceXYZ(int face, float u, float v)
    {
        // Convert UV coordinates to xyz direction for each cube face
        switch (face)
        {
        case 0: return glm::vec3(1.0f, -v, -u);    // Positive X
        case 1: return glm::vec3(-1.0f, -v, u);    // Negative X
        case 2: return glm::vec3(u, 1.0f, v);      // Positive Y
        case 3: return glm::vec3(u, -1.0f, -v);    // Negative Y
        case 4: return glm::vec3(u, -v, 1.0f);     // Positive Z
        case 5: return glm::vec3(-u, -v, -1.0f);   // Negative Z
        default: return glm::vec3(0.0f);
        }
    }

    void convertEquirectangularToFace(float* equirectangular, int equiWidth, int equiHeight, float* faceData, int faceSize, int face)
    {
        for (int y = 0; y < faceSize; y++)
        {
            for (int x = 0; x < faceSize; x++)
            {
                // Convert face coordinates to [-1, 1] range
                float u = (2.0f * x / faceSize) - 1.0f;
                float v = (2.0f * y / faceSize) - 1.0f;

                // Get direction vector for this face pixel
                glm::vec3 dir = getFaceXYZ(face, u, v);
                dir = glm::normalize(dir);

                // Convert xyz direction to spherical coordinates
                float phi = atan2(dir.z, dir.x);
                float theta = asin(dir.y);

                // Convert spherical coordinates to equirectangular UV
                float equiU = (phi + M_PI) / (2.0f * M_PI);
                float equiV = (theta + M_PI / 2.0f) / M_PI;

                // Sample equirectangular map
                int equiX = static_cast<int>(equiU * equiWidth) % equiWidth;
                int equiY = static_cast<int>(equiV * equiHeight) % equiHeight;

                // Copy RGB values
                for (int c = 0; c < 3; c++) {
                    faceData[(y * faceSize + x) * 3 + c] =
                        equirectangular[(equiY * equiWidth + equiX) * 3 + c];
                }
            }
        }
    }

    const char* HDRToCubemapComputeShaderSource = R"(
#version 460

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0, rgba16f) uniform readonly image2D equirectangularMap;
layout(binding = 1, rgba16f) uniform writeonly imageCube cubemapOutput;

layout(location = 0) uniform int cubemapSize;
layout(location = 1) uniform ivec2 equiSize;

const float PI = 3.14159265359;

// Convert UV coordinates to direction vector for each cube face
vec3 getFaceDirection(int face, vec2 uv) {
    switch (face) {
        case 0: return vec3( 1.0,  -uv.y,  -uv.x);    // +X
        case 1: return vec3(-1.0,  -uv.y,   uv.x);    // -X
        case 2: return vec3( uv.x,  1.0,    uv.y);    // +Y
        case 3: return vec3( uv.x, -1.0,   -uv.y);    // -Y
        case 4: return vec3( uv.x, -uv.y,   1.0);     // +Z
        case 5: return vec3(-uv.x, -uv.y,  -1.0);     // -Z
        default: return vec3(0.0);
    }
}

void main() {
    ivec3 pixel = ivec3(gl_GlobalInvocationID.xyz);
    
    // Skip if outside dimensions
    if (pixel.x >= cubemapSize || pixel.y >= cubemapSize) {
        return;
    }
    
    // Convert pixel coordinates to UV in [-1, 1] range
    vec2 uv = 2.0 * vec2(pixel.xy) / float(cubemapSize - 1) - 1.0;
    
    // Get direction vector for this face pixel
    vec3 dir = getFaceDirection(pixel.z, uv);
    dir = normalize(dir);
    
    // Convert direction to spherical coordinates
    float phi = atan(dir.z, dir.x);
    float theta = asin(dir.y);
    
    // Convert to equirectangular UV coordinates
    vec2 equiUV = vec2(
        (phi + PI) / (2.0 * PI),
        (theta + PI/2.0) / PI
    );
    
    // Sample equirectangular map
    vec2 equiPixel = equiUV * vec2(equiSize);
    vec4 color = imageLoad(equirectangularMap, ivec2(equiPixel));
    
    // Write to cubemap face
    imageStore(cubemapOutput, pixel, color);
}
)";
}

namespace PIX3D
{
    namespace GL
    {

        void PIX3D::GL::GLHdriCubemap::LoadHdrToCubemapCPU(const std::filesystem::path& path, uint32_t size)
        {
            if (m_Handle)
                Destroy();

            PIX_ASSERT_MSG((path.extension().string() == ".hdr"), "file is not .hdr file!");

            int width, height, channels;
            stbi_set_flip_vertically_on_load(true);
            float* data = stbi_loadf(path.string().c_str(), &width, &height, &channels, 3);
            stbi_set_flip_vertically_on_load(false);

            PIX_ASSERT_MSG(data, "failed to load hdr file!");

            // Create OpenGL cubemap texture
            glGenTextures(1, &m_Handle);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_Handle);

            // Set texture parameters
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Allocate memory for a single face
            std::vector<float> faceData(size * size * 3);

            // Generate and upload each face
            for (int face = 0; face < 6; face++) {
                convertEquirectangularToFace(
                    data, width, height,
                    faceData.data(), size, face
                );

                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                    0, GL_RGB16F,
                    size, size,
                    0, GL_RGB, GL_FLOAT,
                    faceData.data()
                );
            }

            // Free the original HDRI data
            stbi_image_free(data);
        }

        void GLHdriCubemap::Destroy()
        {
            glDeleteTextures(1, &m_Handle);
            m_Handle = 0;

            glDeleteTextures(1, &m_EquiRectangularHdrTextureHandle);
            m_EquiRectangularHdrTextureHandle = 0;
        }

        void GLHdriCubemap::Bind() const
        {
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_Handle);
        }

        void GLHdriCubemap::LoadHdrToCubemapGPU(const std::filesystem::path& path, uint32_t size)
        {
            if (m_Handle)
                Destroy();

            PIX_ASSERT_MSG((path.extension().string() == ".hdr"), "File is not .hdr file!");

            // Load HDR image
            int width, height, channels;
            stbi_set_flip_vertically_on_load(true);
            float* data = stbi_loadf(path.string().c_str(), &width, &height, &channels, 4); // Load with 4 channels
            stbi_set_flip_vertically_on_load(false);

            PIX_ASSERT_MSG(data, "Failed to load HDR file!");

            // Create and setup equirectangular texture
            glGenTextures(1, &m_EquiRectangularHdrTextureHandle);
            glBindTexture(GL_TEXTURE_2D, m_EquiRectangularHdrTextureHandle);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Create cubemap texture
            glGenTextures(1, &m_Handle);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_Handle);

            // Initialize all faces
            for (int face = 0; face < 6; face++) {
                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                    0, GL_RGBA16F,
                    size, size,
                    0, GL_RGBA, GL_FLOAT,
                    nullptr
                );
            }

            // Set cubemap parameters
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Create and compile compute shader
            GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
            const char* shaderSource = HDRToCubemapComputeShaderSource;
            glShaderSource(computeShader, 1, &shaderSource, nullptr);
            glCompileShader(computeShader);

            // Check compilation
            GLint success;
            glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
            if (!success) {
                char infoLog[512];
                glGetShaderInfoLog(computeShader, 512, nullptr, infoLog);
                std::cerr << "Compute Shader Compilation Failed:\n" << infoLog << std::endl;
                glDeleteShader(computeShader);
                glDeleteTextures(1, &m_EquiRectangularHdrTextureHandle);
                stbi_image_free(data);
                return;
            }

            // Create and link program
            GLuint computeProgram = glCreateProgram();
            glAttachShader(computeProgram, computeShader);
            glLinkProgram(computeProgram);

            // Check linking
            glGetProgramiv(computeProgram, GL_LINK_STATUS, &success);
            if (!success) {
                char infoLog[512];
                glGetProgramInfoLog(computeProgram, 512, nullptr, infoLog);
                std::cerr << "Compute Program Linking Failed:\n" << infoLog << std::endl;
                glDeleteShader(computeShader);
                glDeleteProgram(computeProgram);
                glDeleteTextures(1, &m_EquiRectangularHdrTextureHandle);
                stbi_image_free(data);
                return;
            }

            // Setup compute shader
            glUseProgram(computeProgram);

            // Bind textures
            glBindImageTexture(0, m_EquiRectangularHdrTextureHandle, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
            glBindImageTexture(1, m_Handle, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

            // Set uniforms
            glUniform1i(glGetUniformLocation(computeProgram, "cubemapSize"), size);
            glUniform2i(glGetUniformLocation(computeProgram, "equiSize"), width, height);

            // Dispatch compute shader
            int workGroupSize = 16;
            int numGroups = (size + workGroupSize - 1) / workGroupSize;
            glDispatchCompute(numGroups, numGroups, 6);

            // Wait for completion
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            // Cleanup
            // glDeleteTextures(1, &m_EquiRectangularHdrTextureHandle);
            glDeleteShader(computeShader);
            glDeleteProgram(computeProgram);
            stbi_image_free(data);
        }

        void GLHdriCubemap::Load(uint32_t cubemaphandle)
        {
            m_Handle = cubemaphandle;
        }
    }
}
