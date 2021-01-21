using System;
using System.Runtime.InteropServices;

namespace PixelsForGlory
{
    class RayTracingPlugin
    {
        [DllImport("RayTracingPlugin")]
        public static extern void SetTimeFromUnity(float t);

        [DllImport("RayTracingPlugin")]
        public static extern void SetTargetTexture(IntPtr texture, int width, int height);

        [DllImport("RayTracingPlugin")]
        public static extern int GetSharedMeshIndex(int sharedMeshInstanceId);

        [DllImport("RayTracingPlugin")]
        public static extern int AddMesh(int sharedMeshInstanceId, IntPtr vertices, IntPtr normals, IntPtr uvs, int vertexCount, IntPtr indices, int indexCount);

        [DllImport("RayTracingPlugin")]
        public static extern int AddTlasInstance(int sharedMeshIndex, IntPtr l2wMatrix);

        [DllImport("RayTracingPlugin")]
        public static extern void BuildTlas();

        [DllImport("RayTracingPlugin")]
        public static extern void Prepare(int width, int height);

        [DllImport("RayTracingPlugin")]
        public static extern void UpdateCamera(IntPtr camPos, IntPtr camDir, IntPtr camUp, IntPtr camSide, IntPtr camNearFarFov);

        [DllImport("RayTracingPlugin")]
        public static extern void UpdateSceneData(IntPtr color);

        [DllImport("RayTracingPlugin")]
        public static extern IntPtr GetRenderEventFunc();
    }
}
