#pragma once
#include "GLObject.h"
#include <Core/Buffer.hpp>


namespace PIX3D
{
	namespace GL
	{
		class GLUniformBuffer : GLObject
		{
		public:
			void Create(uint32_t bindingpoint);
			void Update(const PIX3D::BufferData& buffer);
			void Destroy();
		private:

		};
	}
}
