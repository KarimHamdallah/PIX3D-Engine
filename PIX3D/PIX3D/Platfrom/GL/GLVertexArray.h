#pragma once
#include "GLVertexBuffer.h"
#include "GLIndexBuffer.h"
#include "GLDynamicVertexBuffer.h"

namespace PIX3D
{
	namespace GL
	{

		enum class GLShaderAttribType { FLOAT, INT };

		struct GLVertexAttrib
		{
			uint32_t ShaderLocation;
			uint32_t Count;
			GLShaderAttribType Type;
			uint32_t Offset;
		};

		class GLVertexArray : GLObject
		{
		public:
			void Create();
			void Destroy();

			void AddVertexBuffer(GLVertexBuffer& vertexbuffer, uint32_t stride);
			void AddIndexBuffer(GLIndexBuffer& indexbuffer);
			void AddVertexAttrib(const GLVertexAttrib& attrib, uint32_t vertexbufferbindingindex = 0);
			void Bind();
		private:
			uint32_t m_VertexBufferIndex = 0;
		};
	}
}
