#include "GLShader.h"
#include <fstream>
#include <sstream>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <Core/Core.h>
#include <filesystem>

namespace PIX3D
{
    namespace GL
    {

        GLShader::~GLShader()
        {
            if (m_ProgramID != 0)
                glDeleteProgram(m_ProgramID);
        }

        bool GLShader::LoadFromFile(const std::string & vertexPath, const std::string & fragmentPath)
        {
            bool same_shader = std::filesystem::path(vertexPath).stem().string() == std::filesystem::path(fragmentPath).stem().string();
            PIX_ASSERT(same_shader);

            m_ShaderName = std::filesystem::path(vertexPath).stem().string();

            std::string vertexCode;
            std::string fragmentCode;

            try
            {
                // Read vertex shader code from file
                std::ifstream vShaderFile(vertexPath);
                std::stringstream vShaderStream;
                vShaderStream << vShaderFile.rdbuf();
                vertexCode = vShaderStream.str();
                PIX_ASSERT_MSG(!vertexCode.empty(), "Vertex shader file is empty");
                vShaderFile.close();

                // Read fragment shader code from file
                std::ifstream fShaderFile(fragmentPath);
                std::stringstream fShaderStream;
                fShaderStream << fShaderFile.rdbuf();
                fragmentCode = fShaderStream.str();
                PIX_ASSERT_MSG(!fragmentCode.empty(), "Fragment shader file is empty");
                fShaderFile.close();
            }
            catch (std::ifstream::failure& e)
            {
                std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
                return false;
            }

            return CompileAndLink(vertexCode, fragmentCode);
        }

        bool GLShader::LoadFromString(const std::string& vertexCode, const std::string& fragmentCode)
        {
            return CompileAndLink(vertexCode, fragmentCode);
        }

        void GLShader::QueryUniforms()
        {
            std::cout << "Shader Name : " << m_ShaderName << "\n";

            GLint uniformCount;
            glGetProgramiv(m_ProgramID, GL_ACTIVE_UNIFORMS, &uniformCount);

            GLchar name[512];
            for (int i = 0; i < uniformCount; ++i)
            {
                GLint size;
                GLenum type;
                glGetActiveUniform(m_ProgramID,
                    (GLuint)i,
                    sizeof(name),
                    NULL,
                    &size,
                    &type,
                    name
                );

                GLint location = glGetUniformLocation(m_ProgramID, name);

                std::cout << "Uniform #" << i
                    << " Name: " << name
                    << " -  Location: " << location
                    << " -  Type: " << type
                    << "\n";
            }
            std::cout << "----------------------------------------------------" << "\n";
        }

        bool GLShader::CompileAndLink(const std::string & vertexCode, const std::string & fragmentCode)
        {
            const char* vShaderCode = vertexCode.c_str();
            const char* fShaderCode = fragmentCode.c_str();

            GLuint vertex, fragment;
            char infoLog[512];

            // Vertex Shader
            int success = 0;
            vertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex, 1, &vShaderCode, NULL);
            glCompileShader(vertex);

            // Check for vertex shader compile errors
            glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(vertex, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
                PIX_ASSERT(false);
            }

            // Fragment Shader
            fragment = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment, 1, &fShaderCode, NULL);
            glCompileShader(fragment);

            // Check for fragment shader compile errors
            success = 0;
            glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(fragment, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
                PIX_ASSERT(false);
            }

            // Shader Program
            m_ProgramID = glCreateProgram();
            glAttachShader(m_ProgramID, vertex);
            glAttachShader(m_ProgramID, fragment);
            glLinkProgram(m_ProgramID);

            // Check for linking errors
            success = 0;
            glGetProgramiv(m_ProgramID, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(m_ProgramID, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
                PIX_ASSERT(false);
            }

            // QueryUniforms();

            // Delete shaders as they're linked into our program now and no longer necessary
            glDeleteShader(vertex);
            glDeleteShader(fragment);

            return true;
        }

        void GLShader::Bind() const
        {
            glUseProgram(m_ProgramID);
        }

        void GLShader::Destroy()
        {
            glDeleteProgram(m_ProgramID);
        }

        void GLShader::SetBool(const std::string& name, bool value) const
        {
            GLint Location = glGetUniformLocation(m_ProgramID, name.c_str());
            //PIX_ASSERT_MSG(Location >= 0, "Unknown Uniform");
            if(Location >= 0)
                glUniform1i(Location, (int)value);
        }

        void GLShader::SetInt(const std::string& name, int value) const
        {
            GLint Location = glGetUniformLocation(m_ProgramID, name.c_str());
            //PIX_ASSERT_MSG(Location >= 0, "Unknown Uniform");
            if(Location >= 0)
                glUniform1i(Location, value);
        }

        void GLShader::SetFloat(const std::string & name, float value) const
        {
            GLint Location = glGetUniformLocation(m_ProgramID, name.c_str());
            //PIX_ASSERT_MSG(Location >= 0, "Unknown Uniform");
            if (Location >= 0)
                glUniform1f(Location, value);
        }

        void GLShader::SetVec2(const std::string& name, const glm::vec2& value) const
        {
            GLint Location = glGetUniformLocation(m_ProgramID, name.c_str());
            //PIX_ASSERT_MSG(Location >= 0, "Unknown Uniform");
            if (Location >= 0)
                glUniform2f(Location, value.x, value.y);
        }

        void GLShader::SetVec3(const std::string& name, const glm::vec3& value) const
        {
            GLint Location = glGetUniformLocation(m_ProgramID, name.c_str());
            //PIX_ASSERT_MSG(Location >= 0, "Unknown Uniform");
            if (Location >= 0)
                glUniform3f(Location, value.x, value.y, value.z);
        }

        void GLShader::SetVec4(const std::string& name, const glm::vec4& value) const
        {
            GLint Location = glGetUniformLocation(m_ProgramID, name.c_str());
            //PIX_ASSERT_MSG(Location >= 0, "Unknown Uniform");
            if (Location >= 0)
                glUniform4f(Location, value.x, value.y, value.z, value.w);
        }

        void GLShader::SetMat4(const std::string& name, const glm::mat4& matrix) const
        {
            GLint Location = glGetUniformLocation(m_ProgramID, name.c_str());
            //PIX_ASSERT_MSG(Location >= 0, "Unknown Uniform");
            if (Location >= 0)
                glUniformMatrix4fv(Location, 1, GL_FALSE, glm::value_ptr(matrix));
        }

        void GLShader::SetMat3(const std::string& name, const glm::mat3& matrix) const
        {
            GLint Location = glGetUniformLocation(m_ProgramID, name.c_str());
            //PIX_ASSERT_MSG(Location >= 0, "Unknown Uniform");
            if (Location >= 0)
                glUniformMatrix3fv(Location, 1, GL_FALSE, glm::value_ptr(matrix));
        }
    }
}
