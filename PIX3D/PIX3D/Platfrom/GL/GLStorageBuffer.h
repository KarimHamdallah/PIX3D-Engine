#pragma once
#include "GLObject.h"
#include <Core/Buffer.hpp>

namespace PIX3D
{
	namespace GL
	{
		class GLStorageBuffer : public GLObject
		{
		public:
			void Create(uint32_t bindingpoint, uint32_t size);
			void FillData(void* data, uint32_t size);
			void Destroy();

			void Bind(uint32_t bindingpoint);
		private:
			void* MapBuffer();
			void UnMapBuffer();
		private:
			uint32_t m_Size; // size in bytes
		};
	}
}
