using UnityEngine;
using UnityEngine.Rendering;

[CreateAssetMenu(menuName = "Rendering/Ray Tracing Render Pipeline")]
public class RayTracingRenderPipelineAsset : RenderPipelineAsset
{
    protected override RenderPipeline CreatePipeline()
    {
        PixelsForGlory.RayTracingPlugin.Prepare();
        return new RayTracingRenderPipeline();
    }
}
    