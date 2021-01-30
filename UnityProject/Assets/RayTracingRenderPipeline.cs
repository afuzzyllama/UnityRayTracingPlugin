using UnityEngine;
using UnityEngine.Rendering;

using System.Runtime.InteropServices;

public class RayTracingRenderPipeline : RenderPipeline
{
    RayTracingCameraRenderer renderer = new RayTracingCameraRenderer();
    
    protected override void Render(ScriptableRenderContext context, Camera[] cameras)
    {
        // Make sure tlas is built or updated before rendering
        PixelsForGlory.RayTracingPlugin.BuildTlas();

        var colorHandle = GCHandle.Alloc(RenderSettings.ambientSkyColor, GCHandleType.Pinned);
        PixelsForGlory.RayTracingPlugin.UpdateSceneData(colorHandle.AddrOfPinnedObject());
        colorHandle.Free();

        foreach (var camera in cameras)
        {
            renderer.Render(context, camera);
        }

    }
}
