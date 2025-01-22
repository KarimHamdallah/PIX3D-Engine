using System;
using System.Numerics;
using System.Runtime.CompilerServices;

namespace PIX3D
{
    public class PIXEntity
    {
        // This will be set from C++
        public ulong UUID;

        // Virtual functions that C++ will call
        protected virtual void OnCreate() { }
        protected virtual void OnStart() { }
        protected virtual void OnUpdate(float dt) { }
        protected virtual void OnDestroy() { }

        // Transform Methods matching C++ internal calls exactly
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetPosition(ulong entityID, out Vector3 outPosition);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetPosition(ulong entityID, ref Vector3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetRotation(ulong entityID, out Vector3 outRotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetRotation(ulong entityID, ref Vector3 rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetScale(ulong entityID, out Vector3 outScale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetScale(ulong entityID, ref Vector3 scale);
    }

    // Math
    public struct Vector3
    {
        public float X, Y, Z;

        public Vector3(float x, float y, float z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        public static Vector3 Zero => new Vector3(0, 0, 0);
        public static Vector3 One => new Vector3(1, 1, 1);
    }
}
