using System;
using System.Runtime.InteropServices;

using UnityEngine;

namespace PixelsForGlory
{
    internal static class RayTracingPlugin
    {
        [DllImport("RayTracingPlugin", CharSet = CharSet.Unicode)]
        public static extern int SetShaderFolder(string shaderFolder);

        [DllImport("RayTracingPlugin")]
        public static extern int SetRenderTarget(int cameraInstanceId, int unityTextureFormat, int width, int height, IntPtr destination);

        [DllImport("RayTracingPlugin")]
        public static extern int AddSharedMesh(int sharedMeshInstanceId, IntPtr vertices, IntPtr normals, IntPtr uvs, int vertexCount, IntPtr indices, int indexCount);

        [DllImport("RayTracingPlugin")]
        public static extern int AddTlasInstance(int gameObjectInstanceId, int sharedMeshInstanceId, IntPtr l2wMatrix);

        [DllImport("RayTracingPlugin")]
        public static extern void UpdateTlasInstance(int gameObjectInstanceId, IntPtr l2wMatrix);

        [DllImport("RayTracingPlugin")]
        public static extern void RemoveTlasInstance(int gameObjectInstanceId);

        [DllImport("RayTracingPlugin")]
        public static extern void BuildTlas();

        [DllImport("RayTracingPlugin")]
        public static extern void Prepare();

        [DllImport("RayTracingPlugin")]
        public static extern void ResetPipeline();
        
        [DllImport("RayTracingPlugin")]
        public static extern void UpdateCamera(int cameraInstanceId, IntPtr camPos, IntPtr camDir, IntPtr camUp, IntPtr camSide, IntPtr camNearFarFov);

        [DllImport("RayTracingPlugin")]
        public static extern void UpdateSceneData(IntPtr color);

        [DllImport("RayTracingPlugin")]
        public static extern int AddLight(int lightInstanceId, float x, float y, float z, float r, float g, float b, float bounceIntensity, float intensity, float range, float spotAngle, int type, bool enabled);

        [DllImport("RayTracingPlugin")]
        public static extern void UpdateLight(int lightInstanceId, float x, float y, float z, float r, float g, float b, float bounceIntensity, float intensity, float range, float spotAngle, int type, bool enabled);

        [DllImport("RayTracingPlugin")]
        public static extern void RemoveLight(int lightInstanceId);

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
            Watcher.Filter = "*.glsl";

            Watcher.Changed += OnChanged;

            // Start watching
            Watcher.EnableRaisingEvents = true;

            var dirInfo = new System.IO.DirectoryInfo(Application.dataPath);
            RootDir = dirInfo.Parent.Parent.FullName;
        }

        private static void OnChanged(object source, System.IO.FileSystemEventArgs e)
        {
            // TODO, this is so hardcoded it sucks, make it better
            
            string glslCompiler = "glslangValidator.exe";
            string glslDir = RootDir + "\\glslang\\2020.07.28\\bin";
            string sourceFolder = RootDir + "\\PluginSource\\source\\PixelsForGlory\\Shaders";
            string binariesFolder = RootDir + "\\UnityProject\\Assets\\Plugins\\RayTracing\\x86_64";

            var pathParts = e.Name.Split('\\');
            var nameParts = pathParts[pathParts.Length - 1].Split('.');
            var nameOnly = nameParts[0];

            var stageParts = nameOnly.Split('_');
            string stage = $"r{stageParts[1]}";
            
            var glslValidator = $"{glslDir}\\{glslCompiler}";
            var glslPath = $"{sourceFolder}\\{nameOnly}.glsl";

            // What the actual fuck is happening here?
            var binaryPath = $"{binariesFolder}\\{nameOnly}.bin";

            var arguments = $"--target-env vulkan1.2 -V -S {stage} {glslPath} -o {binaryPath}";

            Debug.Log($"Rebuilding shader: {glslValidator} {arguments}");

            var startInfo = new System.Diagnostics.ProcessStartInfo
            {
                CreateNoWindow = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                WorkingDirectory = RootDir,
                FileName = glslValidator,
                Arguments = arguments
            };

            var process = new System.Diagnostics.Process
            {
                StartInfo = startInfo
            };

            process.OutputDataReceived += (sender, args) => { if (args.Data != null) { Debug.Log(args.Data); } };
            process.ErrorDataReceived += (sender, args) => { if (args.Data != null) { Debug.LogError(args.Data); } };

            process.Start();
            process.BeginOutputReadLine();
            process.BeginErrorReadLine();
            process.WaitForExit();

            PixelsForGlory.RayTracingPlugin.ResetPipeline();

        }

        public static void StopMonitoringShaders()
        {
            Debug.Log("Stopping monitoring shaders");
            Watcher.Dispose();
        }

        private static System.IO.FileSystemWatcher Watcher;
    }

}
