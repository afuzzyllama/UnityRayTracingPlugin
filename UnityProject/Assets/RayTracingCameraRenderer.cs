using System;
using UnityEngine;
using UnityEngine.Rendering;

public class RayTracingCameraRenderer
{
    const string bufferName = "Ray Tracing Render Camera";

    Texture2D _target;

    ScriptableRenderContext context;
    Camera camera;

    public void Render(ScriptableRenderContext context, Camera camera)
    {
        this.context = context;
        this.camera = camera;

        Setup();
        RayTrace();
        Submit();
    }

    private void Setup()
    {
        context.SetupCameraProperties(camera);
        
        if (_target == null || _target.width != Screen.width || _target.height != Screen.height)
        {
            // Get a render target for Ray Tracing
            _target = new Texture2D(Screen.width, Screen.height, TextureFormat.ARGB32, false);

            // Set point filtering just so we can see the pixels clearly
            _target.filterMode = FilterMode.Point;
            // Call Apply() so it's actually uploaded to the GPU
            _target.Apply();

            PixelsForGlory.RayTracingPlugin.SetTargetTexture(_target.GetNativeTexturePtr(), _target.width, _target.height);
        }
    }

    private void RayTrace()
    {
        GL.IssuePluginEvent(PixelsForGlory.RayTracingPlugin.GetRenderEventFunc(), 1);
    }

    private void Submit()
    {
        Graphics.Blit(_target, camera.targetTexture);
    }
}
