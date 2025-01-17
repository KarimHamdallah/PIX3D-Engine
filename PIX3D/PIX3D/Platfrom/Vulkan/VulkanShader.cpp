#include "VulkanShader.h"
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <Engine/Engine.hpp>
#include "glslang/Include/glslang_c_interface.h"
#include "glslang/Public/resource_limits_c.h"

namespace PIX3D
{
    namespace VK
    {
        VulkanShader::VulkanShader()
            : m_vertexShader(VK_NULL_HANDLE)
            , m_fragmentShader(VK_NULL_HANDLE)
        {
            m_CachePath = "../PIX3D/res/compiled spriv shaders/";
        }

        VulkanShader::~VulkanShader()
        {
        }

        bool VulkanShader::FileExists(const std::string& filename) {
            std::ifstream file(filename);
            if (file.good())
            {
                file.close();
                return true;
            }
            return false;
        }

        VkShaderModule VulkanShader::LoadBinaryShader(const std::string& filename) {
            if (!FileExists(filename)) {
                return NULL;
            }

            FILE* file;
            fopen_s(&file, filename.c_str(), "rb");
            if (!file)
            {
                return NULL;
            }

            fseek(file, 0, SEEK_END);
            size_t fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            std::vector<char> buffer(fileSize);
            fread(buffer.data(), 1, fileSize, file);
            fclose(file);

            VkShaderModuleCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = buffer.size(),
                .pCode = reinterpret_cast<const uint32_t*>(buffer.data())
            };

            VkShaderModule shaderModule;
            if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
                return NULL;
            }

            return shaderModule;
        }

        std::string VulkanShader::ReadFile(const std::string& filename) {
            if (!FileExists(filename)) {
                throw std::runtime_error("File does not exist: " + filename);
            }

            std::ifstream file(filename, std::ios::ate | std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open file: " + filename);
            }

            size_t fileSize = (size_t)file.tellg();
            std::string buffer(fileSize, ' ');

            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();

            return buffer;
        }

        bool VulkanShader::CompileShader(std::string stage, const char* shaderCode, Shader& shaderModule)
        {
            glslang_stage_t _stage;

            if (stage == "vert")
                _stage = GLSLANG_STAGE_VERTEX;
            else if (stage == "frag")
                _stage = GLSLANG_STAGE_FRAGMENT;
            else if (stage == "comp")
                _stage = GLSLANG_STAGE_COMPUTE;
            else
                PIX_ASSERT_MSG(false, "Unkown Stage!");

            glslang_input_t input = 
            {
                .language = GLSLANG_SOURCE_GLSL,
                .stage = _stage,
                .client = GLSLANG_CLIENT_VULKAN,
                .client_version = GLSLANG_TARGET_VULKAN_1_3,
                .target_language = GLSLANG_TARGET_SPV,
                .target_language_version = GLSLANG_TARGET_SPV_1_3,
                .code = shaderCode,
                .default_version = 100,
                .default_profile = GLSLANG_NO_PROFILE,
                .force_default_version_and_profile = false,
                .forward_compatible = false,
                .messages = GLSLANG_MSG_DEFAULT_BIT,
                .resource = glslang_default_resource()
            };

            glslang_shader_t* shader = glslang_shader_create(&input);

            if (!glslang_shader_preprocess(shader, &input)) {
                fprintf(stderr, "GLSL preprocessing failed\n");
                fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
                return false;
            }

            if (!glslang_shader_parse(shader, &input)) {
                fprintf(stderr, "GLSL parsing failed\n");
                fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
                return false;
            }

            glslang_program_t* program = glslang_program_create();
            glslang_program_add_shader(program, shader);

            if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
                fprintf(stderr, "GLSL linking failed\n");
                fprintf(stderr, "\n%s", glslang_program_get_info_log(program));
                return false;
            }

            glslang_program_SPIRV_generate(program, _stage);
            
            {
                size_t program_size = glslang_program_SPIRV_get_size(program);
                shaderModule.SPIRV.resize(program_size);
                glslang_program_SPIRV_get(program, shaderModule.SPIRV.data());
            }

            VkShaderModuleCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = shaderModule.SPIRV.size() * sizeof(uint32_t),
                .pCode = shaderModule.SPIRV.data()
            };

            VkResult result = vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule.shaderModule);
            if (result != VK_SUCCESS) {
                fprintf(stderr, "Failed to create shader module\n");
                return false;
            }

            glslang_program_delete(program);
            glslang_shader_delete(shader);

            return true;
        }

        bool VulkanShader::LoadFromFile(const std::string& vertPath, const std::string& fragPath, bool always_compile, bool dont_save_binary)
        {
            if (!m_device)
            {
                auto* GraphicsContext = (VulkanGraphicsContext*)Engine::GetGraphicsContext();;
                m_device = GraphicsContext->m_Device;
            }

            if (m_device == VK_NULL_HANDLE)
            {
                fprintf(stderr, "Device not set\n");
                return false;
            }

            // Initialize glslang
            glslang_initialize_process();

            // Check for pre-compiled SPIR-V files
            std::filesystem::path vertpath = vertPath;
            std::filesystem::path fragpath = fragPath;

            std::string vertSpvPath = m_CachePath + vertpath.filename().string() + ".spv";
            std::string fragSpvPath = m_CachePath + fragpath.filename().string() + ".spv";

            bool usePrecompiled = FileExists(vertSpvPath) && FileExists(fragSpvPath);

            if (usePrecompiled && !always_compile)
            {
                // Load pre-compiled shaders
                m_vertexShader = LoadBinaryShader(vertSpvPath);
                m_fragmentShader = LoadBinaryShader(fragSpvPath);

                if (m_vertexShader && m_fragmentShader) {
                    printf("Loaded pre-compiled shaders successfully\n");
                    return true;
                }
            }

            // Compile shaders from source
            try {
                std::string vertSource = ReadFile(vertPath);
                std::string fragSource = ReadFile(fragPath);

                Shader vertShader, fragShader;

                if (!CompileShader("vert", vertSource.c_str(), vertShader) ||
                    !CompileShader("frag", fragSource.c_str(), fragShader))
                {
                    glslang_finalize_process();
                    return false;
                }

                m_vertexShader = vertShader.shaderModule;
                m_fragmentShader = fragShader.shaderModule;

                // Save compiled SPIR-V
                if (!dont_save_binary)
                {
                    FILE* file;
                    fopen_s(&file, vertSpvPath.c_str(), "wb");
                    if (file)
                    {
                        fwrite(vertShader.SPIRV.data(), sizeof(uint32_t), vertShader.SPIRV.size(), file);
                        fclose(file);
                    }

                    fopen_s(&file, fragSpvPath.c_str(), "wb");
                    if (file)
                    {
                        fwrite(fragShader.SPIRV.data(), sizeof(uint32_t), fragShader.SPIRV.size(), file);
                        fclose(file);
                    }

                    printf("Compiled and saved shaders successfully\n");

                }
                glslang_finalize_process();
                return true;
            }
            catch (const std::exception& e) {
                fprintf(stderr, "Error loading shaders: %s\n", e.what());
                glslang_finalize_process();
                return false;
            }
        }

        bool VulkanShader::LoadComputeShaderFromFile(const std::string& computePath, bool always_compile, bool dont_save_binary)
        {
            if (!m_device)
            {
                auto* GraphicsContext = (VulkanGraphicsContext*)Engine::GetGraphicsContext();;
                m_device = GraphicsContext->m_Device;
            }

            if (m_device == VK_NULL_HANDLE)
            {
                fprintf(stderr, "Device not set\n");
                return false;
            }

            // Initialize glslang
            glslang_initialize_process();

            // Check for pre-compiled SPIR-V file
            std::filesystem::path path = computePath;
            std::string computeSpvPath = m_CachePath + path.filename().string() + ".spv";

            if (FileExists(computeSpvPath) && !always_compile)
            {
                // Load pre-compiled shader
                m_computeShader = LoadBinaryShader(computeSpvPath);
                if (m_computeShader) {
                    printf("Loaded pre-compiled compute shader successfully\n");
                    return true;
                }
            }

            // Compile shader from source
            try {
                std::string computeSource = ReadFile(computePath);
                Shader computeShader;

                if (!CompileShader("comp", computeSource.c_str(), computeShader))
                {
                    glslang_finalize_process();
                    return false;
                }

                m_computeShader = computeShader.shaderModule;

                // Save compiled SPIR-V
                if (!dont_save_binary)
                {
                    FILE* file;
                    fopen_s(&file, computeSpvPath.c_str(), "wb");
                    if (file) {
                        fwrite(computeShader.SPIRV.data(), sizeof(uint32_t), computeShader.SPIRV.size(), file);
                        fclose(file);
                    }

                    printf("Compiled and saved compute shader successfully\n");
                }
                glslang_finalize_process();
                return true;

            }
            catch (const std::exception& e) {
                fprintf(stderr, "Error loading compute shader: %s\n", e.what());
                glslang_finalize_process();
                return false;
            }
        }

        VkShaderModule VulkanShader::GetVertexShader() const {
            return m_vertexShader;
        }

        VkShaderModule VulkanShader::GetFragmentShader() const {
            return m_fragmentShader;
        }

        VkShaderModule VulkanShader::GetComputeShader() const {
            return m_computeShader;
        }

        void VulkanShader::Destroy()
        {
            if (m_device != VK_NULL_HANDLE)
            {
                if (m_vertexShader != VK_NULL_HANDLE)
                {
                    vkDestroyShaderModule(m_device, m_vertexShader, nullptr);
                    m_vertexShader = VK_NULL_HANDLE;
                }

                if (m_fragmentShader != VK_NULL_HANDLE)
                {
                    vkDestroyShaderModule(m_device, m_fragmentShader, nullptr);
                    m_fragmentShader = VK_NULL_HANDLE;
                }

                if (m_computeShader != VK_NULL_HANDLE)
                {
                    vkDestroyShaderModule(m_device, m_computeShader, nullptr);
                    m_computeShader = VK_NULL_HANDLE;
                }

                m_device = VK_NULL_HANDLE;
            }
        }
    }
}
