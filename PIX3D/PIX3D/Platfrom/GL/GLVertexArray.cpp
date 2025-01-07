#include "GLVertexArray.h"
#include <glad/glad.h>
#include <Core/Core.h>

namespace
{
	GLenum ToGL(PIX3D::GL::GLShaderAttribType type)
	{
		switch (type)
		{
		case PIX3D::GL::GLShaderAttribType::FLOAT: return GL_FLOAT;
		case PIX3D::GL::GLShaderAttribType::INT: return GL_INT;
		default:
			PIX_ASSERT_MSG(false, "Unknown Type!");
		}
	}
}

namespace PIX3D
{
	namespace GL
	{
		void GLVertexArray::Create()
		{
			glCreateVertexArrays(1, &m_Handle);
			PIX_ASSERT_MSG(m_Handle, "failed to create vertexarray!");
		}

		void GLVertexArray::Destroy()
		{
			glDeleteVertexArrays(1, &m_Handle);
		}

		void GLVertexArray::AddIndexBuffer(GLIndexBuffer& indexbuffer)
		{
			glVertexArrayElementBuffer(m_Handle, indexbuffer.GetHandle());
		}

		void GLVertexArray::AddVertexBuffer(GLVertexBuffer& vertexbuffer, uint32_t stride)
		{
			glVertexArrayVertexBuffer(m_Handle, m_VertexBufferIndex++, vertexbuffer.GetHandle(), 0, stride);
		}

		void GLVertexArray::AddVertexAttrib(const GLVertexAttrib& attrib, uint32_t vertexbufferbindingindex)
		{
			glVertexArrayAttribFormat(m_Handle, attrib.ShaderLocation, attrib.Count, ToGL(attrib.Type), false, attrib.Offset);
			glVertexArrayAttribBinding(m_Handle, attrib.ShaderLocation, vertexbufferbindingindex);

			glEnableVertexArrayAttrib(m_Handle, attrib.ShaderLocation);
		}

		void GLVertexArray::Bind()
		{
			glBindVertexArray(m_Handle);
		}
	}
}
