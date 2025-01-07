#pragma once
#include <stdint.h>

namespace PIX3D
{
	namespace GL
	{
		enum class ClearFlags { COLOR, DEPTH, COLOR_DEPTH };
		enum class Primitive { LINES, LINE_LOOP, TRIANGLES };

		class GLCommands
		{
		public:

			static void Clear(float r, float g, float b, float a);
			static void ClearFlag(ClearFlags flag);
			static void SetViewPort(uint32_t width, uint32_t height);
			static void DrawArrays(Primitive primitive, uint32_t count);
			static void DrawIndexed(Primitive primitive, uint32_t count);
			static void DrawIndexedBaseVertex(Primitive primitive, uint32_t count, uint32_t baseindex, uint32_t basevertex);
			static void FillPolygon(bool fill);
		};
	}
}
