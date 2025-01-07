#include "GLDynamicVertexBuffer.h"
#include <glad/glad.h>
#include <Core/Core.h>

namespace PIX3D
{
	namespace GL
	{

		void GLDynamicVertexBuffer::Create()
		{
			glGenBuffers(1, &m_Handle);
			PIX_ASSERT_MSG(m_Handle, "failed to create buffer!");

			glBindBuffer(GL_ARRAY_BUFFER, m_Handle);
			glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
		}

		void GLDynamicVertexBuffer::UpdateData(const PIX3D::BufferData& buffer)
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_Handle);
			glBufferSubData(GL_ARRAY_BUFFER, 0, buffer.GetSize(), buffer.GetData());
		}

		void GLDynamicVertexBuffer::Destroy()
		{
			glDeleteBuffers(1, &m_Handle);
		}
	}
}
