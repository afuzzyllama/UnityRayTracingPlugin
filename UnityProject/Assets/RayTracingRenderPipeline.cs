using UnityEngine;
using UnityEngine.Rendering;

using System.Runtime.InteropServices;
using System;

public class RayTracingRenderPipeline : RenderPipeline
{
    RayTracingCameraRenderer renderer = new RayTracingCameraRenderer();
        
    protected override void Render(ScriptableRenderContext context, Camera[] cameras)
    {
        foreach (var camera in cameras)
        {
            renderer.Render(context, camera);
        }

    }
}
