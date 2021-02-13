using UnityEngine;
using UnityEngine.Rendering;

using System.IO;

namespace PixelsForGlory.RayTracing
{
    [CreateAssetMenu(menuName = "Pixels for Glory/Ray Tracing/Ray Tracing Render Pipeline")]
    public class RayTracingRenderPipelineAsset : RenderPipelineAsset
    {
        protected override RenderPipeline CreatePipeline()
        {
            PixelsForGlory.RayTracing.RayTracingPlugin.SetShaderFolder(Path.GetFullPath("Packages/com.pixelsforglory.raytracing/Runtime/Plugins/x86_64"));
            PixelsForGlory.RayTracing.RayTracingPlugin.MonitorShaders(Path.GetFullPath("Packages/com.pixelsforglory.raytracing/Runtime/Plugins/x86_64"));
            PixelsForGlory.RayTracing.RayTracingPlugin.Prepare();
            return new RayTracingRenderPipeline();
        }

        protected override void OnDisable()
        {
            base.OnDisable();
            PixelsForGlory.RayTracing.RayTracingPlugin.StopMonitoringShaders();
        }
    }
}    