#include "GLVertexBuffer.h"
#include <glad/glad.h>
#include <Core/Core.h>

namespace
{
	GLenum ToGL(PIX3D::GL::GLVertexBufferUsage usage)
	{
		switch (usage)
		{
		case PIX3D::GL::GLVertexBufferUsage::STATIC_DRAW: return GL_STATIC_DRAW;
		case PIX3D::GL::GLVertexBufferUsage::DYNAMIC_DRAW: return GL_DYNAMIC_DRAW;
		default:
			PIX_ASSERT_MSG(false, "UnKnown Usage!");
		}
	}
}
namespace PIX3D
{
	namespace GL
	{
		void GLVertexBuffer::Create()
		{
			glGenBuffers(1, &m_Handle);
			PIX_ASSERT_MSG(m_Handle, "failed to create buffer!");
		}

		void GLVertexBuffer::Fill(const PIX3D::BufferData& buffer, GLVertexBufferUsage usage)
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_Handle);
			glBufferData(GL_ARRAY_BUFFER, buffer.GetSize(), buffer.GetData(), ToGL(usage));
		}

		void GLVertexBuffer::Update(const PIX3D::BufferData& buffer)
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_Handle);
			glBufferSubData(GL_ARRAY_BUFFER, 0, buffer.GetSize(), buffer.GetData());
		}

		void GLVertexBuffer::Destroy()
		{
			glDeleteBuffers(1, &m_Handle);
		}
	}
}
