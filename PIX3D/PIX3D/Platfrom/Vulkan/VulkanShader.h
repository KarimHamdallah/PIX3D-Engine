#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace PIX3D
{
    namespace VK
    {
        class VulkanShader
        {
        public:
            VulkanShader();
            ~VulkanShader();

            bool LoadFromFile(const std::string& vertPath, const std::string& fragPath, bool always_compile = true, bool dont_save_binary = true);
            bool LoadComputeShaderFromFile(const std::string& computePath);
            
            VkShaderModule GetVertexShader() const;
            VkShaderModule GetFragmentShader() const;
            VkShaderModule GetComputeShader() const;

        private:
            VkDevice m_device;
            VkShaderModule m_vertexShader;
            VkShaderModule m_fragmentShader;
            VkShaderModule m_computeShader;
            std::string m_CachePath;

            struct Shader
            {
                std::vector<uint32_t> SPIRV;
                VkShaderModule shaderModule = NULL;
            };

            bool CompileShader(std::string stage, const char* shaderCode, Shader& shaderModule);
            VkShaderModule LoadBinaryShader(const std::string& filename);
            std::string ReadFile(const std::string& filename);
            bool FileExists(const std::string& filename);
        };
    }
}
