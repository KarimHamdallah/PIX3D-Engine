#pragma once
#include "GLObject.h"
#include <vector>

namespace PIX3D
{
	namespace GL
	{
		class GLIndexBuffer : public GLObject
		{
		public:
			void Create();
			void Fill(const std::vector<uint32_t>& indices);
			void Destroy();
		};
	}
}
