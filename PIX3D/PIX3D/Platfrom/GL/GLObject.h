#pragma once
#include <stdint.h>

namespace PIX3D
{
	namespace GL
	{
		class GLObject
		{
		public:
			uint32_t GetHandle() const { return m_Handle; }
			bool IsValid() const { return m_Handle > 0; }
		protected:
			uint32_t m_Handle = 0;
		};
	}
}
