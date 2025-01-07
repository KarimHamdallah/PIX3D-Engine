#pragma once
#include "GLObject.h"
#include <Core/Buffer.hpp>

namespace PIX3D
{
	namespace GL
	{
		enum class GLVertexBufferUsage { STATIC_DRAW, DYNAMIC_DRAW };

		class GLVertexBuffer : public GLObject
		{
		public:
			void Create();
			void Fill(const PIX3D::BufferData& buffer, GLVertexBufferUsage usage = GLVertexBufferUsage::STATIC_DRAW);
			void Update(const PIX3D::BufferData& buffer);
			void Destroy();
		};
	}
}
