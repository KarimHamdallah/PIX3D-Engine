using System;
using System.Runtime.CompilerServices;

namespace PIX3D
{
    public class PIXEntity
    {
        // This will be set from C++
        protected ulong UUID;

        // Virtual functions that C++ will call
        protected virtual void OnCreate() { }
        protected virtual void OnStart() { }
        protected virtual void OnUpdate(float dt) { }
        protected virtual void OnDestroy() { }

    }
}
