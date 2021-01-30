using UnityEngine;
using UnityEngine.Rendering;

using System.Collections.Generic;
using System.Runtime.InteropServices;

public class RayTracingCameraRenderer
{
    private ScriptableRenderContext _context;
    private Camera _camera;
    private Texture2D _target;

    private CommandBuffer _commandBuffer = new CommandBuffer()
    {
        name = "Ray Tracing Camera Render"    
    };

    public void Render(ScriptableRenderContext context, Camera camera)
    {
        _context = context;
        _camera = camera;

        if (!Setup())
        {
            return;
        }

        RayTrace();
    }

    private bool Setup()
    {
        var camPosHandle = GCHandle.Alloc(_camera.transform.position, GCHandleType.Pinned);
        var camDirHandle = GCHandle.Alloc(_camera.transform.forward, GCHandleType.Pinned);
        var camUpHandle = GCHandle.Alloc(_camera.transform.up, GCHandleType.Pinned);
        var camSideHandle = GCHandle.Alloc(_camera.transform.right, GCHandleType.Pinned);
        var camNearFarFovHandle = GCHandle.Alloc(new Vector3(_camera.nearClipPlane, _camera.farClipPlane, _camera.fieldOfView), GCHandleType.Pinned);
        PixelsForGlory.RayTracingPlugin.UpdateCamera(camPosHandle.AddrOfPinnedObject(),
                                                     camDirHandle.AddrOfPinnedObject(),
                                                     camUpHandle.AddrOfPinnedObject(),
                                                     camSideHandle.AddrOfPinnedObject(),
                                                     camNearFarFovHandle.AddrOfPinnedObject());
        camPosHandle.Free();
        camDirHandle.Free();
        camUpHandle.Free();
        camSideHandle.Free();
        camNearFarFovHandle.Free();

        // TODO: PLAN OF ATTACK
        //  - pass target to plugin, set descriptor set and render target for currentFrameNumber
        //  - when tracing is complete, free descriptor set and copy output to render target

        int width;
        int height;
        if(_camera.activeTexture == null)
        {
            width = Screen.width;
            height = Screen.height;
        }
        else
        {
            width = _camera.activeTexture.width;
            height = _camera.activeTexture.height;
        }

        _target = new Texture2D(width, height, TextureFormat.RGBA32, false)
        {
            // Set point filtering just so we can see the pixels clearly
            filterMode = FilterMode.Point
        };

        // Call Apply() so it's actually uploaded to the GPU
        _target.Apply();

        if (PixelsForGlory.RayTracingPlugin.SetRenderTarget((int)_target.format, _target.width, _target.height, _target.GetNativeTexturePtr()) == 0)
        {
            Debug.Log("Something went wrong with setting render target");
            return false;
        }

        return true;
    }

    private void RayTrace()
    {
        _commandBuffer.Clear();

        var cameraTextureId = new RenderTargetIdentifier(_camera.targetTexture);

        string sampleName = "Trace rays";
        _commandBuffer.BeginSample(sampleName);
        _commandBuffer.IssuePluginEvent(PixelsForGlory.RayTracingPlugin.GetRenderEventFunc(), 1);
        _commandBuffer.Blit(_target, cameraTextureId);
        _commandBuffer.EndSample(sampleName);

        _context.ExecuteCommandBuffer(_commandBuffer);
        _context.Submit();

        _commandBuffer.Clear();

 
    }
}
