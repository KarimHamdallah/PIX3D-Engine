#include "GLStorageBuffer.h"
#include <glad/glad.h>
#include <Core/Core.h>

namespace PIX3D
{
	namespace GL
	{
		void GLStorageBuffer::Create(uint32_t bindingpoint, uint32_t size)
		{
			m_Size = size;

			glGenBuffers(1, &m_Handle);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_Handle);
			// Create buffer
			glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_COPY);
		}

		void* GLStorageBuffer::MapBuffer()
		{
			return (void*)glMapBufferRange(m_Handle, 0, m_Size, GL_MAP_WRITE_BIT);
		}

		void GLStorageBuffer::UnMapBuffer()
		{
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}

		void GLStorageBuffer::FillData(void* data, uint32_t size)
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_Handle);

			glMemoryBarrier(GL_ALL_BARRIER_BITS);  // Don't know if this is necessary, just a precaution
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);  // Don't know if this is necessary, just a precaution
		}

		void GLStorageBuffer::Destroy()
		{
			glDeleteBuffers(1, &m_Handle);
		}

		void GLStorageBuffer::Bind(uint32_t bindingpoint)
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_Handle);
			// Now bind the buffer to the shader
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingpoint, m_Handle);
		}
	}
}
