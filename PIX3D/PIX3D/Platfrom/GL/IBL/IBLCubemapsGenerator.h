#pragma once
#include "../GLShader.h"
#include "GLHdrCubemap.h"

namespace PIX3D
{
    namespace GL
    {
        struct IBLMaps
        {
            GLHdriCubemap IrradianceMap;
            GLHdriCubemap PrefilterMap;
            int IrradianceSize = 0;
            int PrefilterSize = 0;

            void Destroy()
            {
                IrradianceMap.Destroy();
                PrefilterMap.Destroy();
                IrradianceSize = 0;
                PrefilterSize = 0;
            }
        };

        class IBLCubemapsGenerator
        {
        public:
            static void Init();
            static void Destroy();
            static IBLMaps GenerateIBLMaps(uint32_t envcubemap, int irradiancesize, int prefiltersize);

            inline static uint32_t GetBrdfLUTTextureHandle() { return s_BrdfLUTTextureHandle; }

        private:
            static uint32_t GenerateBRDFLUTTexture(unsigned int resolution = 512);
        private:
            inline static GLShader s_DiffuseIrradianceShader;
            inline static GLShader s_SpecularIBLShader;
            inline static GLShader s_BrdfLUTShader;
            inline static uint32_t s_BrdfLUTTextureHandle = 0;
        };
    }
}
