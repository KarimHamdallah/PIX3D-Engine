#pragma once
#include "GLObject.h"
#include <Core/Buffer.hpp>

namespace PIX3D
{
	namespace GL
	{
		class GLDynamicVertexBuffer : public GLObject
		{
		public:
			void Create();
			void UpdateData(const PIX3D::BufferData& buffer);
			void Destroy();
		};
	}
}
