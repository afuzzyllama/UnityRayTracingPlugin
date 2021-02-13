using System;
using System.Runtime.InteropServices;

using UnityEngine;

namespace PixelsForGlory.RayTracing
{
    internal static class RayTracingPlugin
    {
        [DllImport("RayTracingPlugin", CharSet = CharSet.Unicode)]
        public static extern int SetShaderFolder(string shaderFolder);

        [DllImport("RayTracingPlugin")]
        public static extern int SetRenderTarget(int cameraInstanceId, int unityTextureFormat, int width, int height, IntPtr destination);

        [DllImport("RayTracingPlugin")]
        public static extern int AddSharedMesh(int sharedMeshInstanceId, IntPtr vertices, IntPtr normals, IntPtr tangets, IntPtr uvs, int vertexCount, IntPtr indices, int indexCount);

        [DllImport("RayTracingPlugin")]
        public static extern int AddTlasInstance(int gameObjectInstanceId, int sharedMeshInstanceId, int materialInstanceId, IntPtr l2wMatrix, IntPtr w2lMatrix);

        [DllImport("RayTracingPlugin")]
        public static extern void UpdateTlasInstance(int gameObjectInstanceId, int materialInstanceId, IntPtr l2wMatrix, IntPtr w2lMatrix);

        [DllImport("RayTracingPlugin")]
        public static extern void RemoveTlasInstance(int gameObjectInstanceId);

        [DllImport("RayTracingPlugin")]
        public static extern void Prepare();

        [DllImport("RayTracingPlugin")]
        public static extern void ResetPipeline();
        
        [DllImport("RayTracingPlugin")]
        public static extern void UpdateCamera(int cameraInstanceId, IntPtr camPos, IntPtr camDir, IntPtr camUp, IntPtr camSide, float camNear, float camFar, float camFov);

        [DllImport("RayTracingPlugin")]
        public static extern void UpdateSceneData(IntPtr color);

        [DllImport("RayTracingPlugin")]
        public static extern int AddLight(int lightInstanceId, float x, float y, float z, float r, float g, float b, float bounceIntensity, float intensity, float range, float spotAngle, int type, bool enabled);

        [DllImport("RayTracingPlugin")]
        public static extern void UpdateLight(int lightInstanceId, float x, float y, float z, float r, float g, float b, float bounceIntensity, float intensity, float range, float spotAngle, int type, bool enabled);

        [DllImport("RayTracingPlugin")]
        public static extern void RemoveLight(int lightInstanceId);

        [DllImport("RayTracingPlugin")]
        public static extern int AddTexture(int textureInstanceId, IntPtr texture);

        [DllImport("RayTracingPlugin")]
        public static extern void RemoveTexture(int textureInstanceId);

        [DllImport("RayTracingPlugin")]
        public static extern int AddMaterial(int materialInstanceId,
                                             float albedo_r, float albedo_g, float albedo_b,
                                             float emission_r, float emission_g, float emission_b,
                                             float transmittance_r, float transmittance_g, float transmittance_b,
                                             float metallic,
                                             float roughness,
                                             float indexOfRefraction,
                                             bool albedoTextureSet,
                                             int albedoTextureInstanceId,
                                             bool emissionTextureSet,
                                             int emissionTextureInstanceId,
                                             bool normalTextureSet,
                                             int normalTextureInstanceId,
                                             bool metallicTextureSet,
                                             int metallicTextureInstanceId,
                                             bool roughnessTextureSet,
                                             int roughnessTextureInstanceId,
                                             bool ambientOcclusionTextureSet,
                                             int ambientOcclusionTextureInstanceId);
        [DllImport("RayTracingPlugin")]
        public static extern void UpdateMaterial(int materialInstanceId,
                                                 float albedo_r, float albedo_g, float albedo_b,
                                                 float emission_r, float emission_g, float emission_b,
                                                 float transmittance_r, float transmittance_g, float transmittance_b,
                                                 float metallic,
                                                 float roughness,
                                                 float indexOfRefraction,
                                                 bool albedoTextureSet,
                                                 int albedoTextureInstanceId,
                                                 bool emissionTextureSet,
                                                 int emissionTextureInstanceId,
                                                 bool normalTextureSet,
                                                 int normalTextureInstanceId,
                                                 bool metallicTextureSet,
                                                 int metallicTextureInstanceId,
                                                 bool roughnessTextureSet,
                                                 int roughnessTextureInstanceId,
                                                 bool ambientOcclusionTextureSet,
                                                 int ambientOcclusionTextureInstanceId);

        [DllImport("RayTracingPlugin")]
        public static extern void RemoveMaterial(int materialInstanceId);

        [StructLayout(LayoutKind.Sequential)]
        public struct RayTracerStatistics
        {
            public uint RegisteredLights;
            public uint RegisteredSharedMeshes;
            public uint RegisteredMeshInstances;
            public uint RegisteredMaterials;
            public uint RegisteredTextures;
            public uint RegisteredRenderTargets;

            public uint DescriptorSetCount;
            public uint AccelerationStuctureCount;
            public uint UniformBufferCount;
            public uint StorageImageCount;
            public uint StorageBufferCount;
            public uint CombinedImageSamplerCount;
        }

        [DllImport("RayTracingPlugin")]
        public static extern RayTracerStatistics GetRayTracerStatistics();

        [DllImport("RayTracingPlugin")]
        public static extern IntPtr GetEventFunc();

        [DllImport("RayTracingPlugin")]
        public static extern IntPtr GetEventAndDataFunc();

        private static string RootDir;

        public static void MonitorShaders(string sourcePath)
        {
            Debug.Log("Monitoring shaders");
            Watcher = new System.IO.FileSystemWatcher();
            Watcher.Path = sourcePath;

            Watcher.NotifyFilter = System.IO.NotifyFilters.LastWrite;
            Watcher.Filter = "*.bin";

            Watcher.Changed += OnChanged;

            // Start watching
            Watcher.EnableRaisingEvents = true;

            var dirInfo = new System.IO.DirectoryInfo(Application.dataPath);
            RootDir = dirInfo.Parent.Parent.FullName;
        }

        private static void OnChanged(object source, System.IO.FileSystemEventArgs e)
        {
            PixelsForGlory.RayTracing.RayTracingPlugin.ResetPipeline();

        }

        public static void StopMonitoringShaders()
        {
            Debug.Log("Stopping monitoring shaders");
            Watcher.Dispose();
        }

        private static System.IO.FileSystemWatcher Watcher;
    }

}
