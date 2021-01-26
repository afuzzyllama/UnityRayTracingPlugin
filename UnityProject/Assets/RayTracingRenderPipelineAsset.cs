using UnityEngine;
using UnityEngine.Rendering;

[CreateAssetMenu(menuName = "Rendering/Ray Tracing Render Pipeline")]
public class RayTracingRenderPipelineAsset : RenderPipelineAsset
{
    protected override RenderPipeline CreatePipeline()
    {
        PixelsForGlory.RayTracingPlugin.SetShaderFolder(System.IO.Path.Combine(Application.dataPath, "Plugins", "RayTracing", "x86_64"));
        PixelsForGlory.RayTracingPlugin.Prepare();
        return new RayTracingRenderPipeline();
    }
}
    