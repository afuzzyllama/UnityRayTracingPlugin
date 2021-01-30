using UnityEngine;
using UnityEngine.Rendering;

using System.Collections.Generic;
using System.Runtime.InteropServices;

public class RayTracingCameraRenderer
{
    enum Events : int
    {
        None        = 0,
        TraceRays   = 1
    };

    private ScriptableRenderContext _context;
    private Camera _camera;

    Dictionary<int, Texture2D> _targets = new Dictionary<int, Texture2D>();

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
        var up = -_camera.transform.up;

        var camPosHandle = GCHandle.Alloc(_camera.transform.position, GCHandleType.Pinned);
        var camDirHandle = GCHandle.Alloc(_camera.transform.forward, GCHandleType.Pinned);
        var camUpHandle = GCHandle.Alloc(up, GCHandleType.Pinned);
        var camSideHandle = GCHandle.Alloc(_camera.transform.right, GCHandleType.Pinned);
        var camNearFarFovHandle = GCHandle.Alloc(new Vector3(_camera.nearClipPlane, _camera.farClipPlane, Mathf.Deg2Rad * _camera.fieldOfView), GCHandleType.Pinned);
        PixelsForGlory.RayTracingPlugin.UpdateCamera(_camera.GetInstanceID(),
                                                     camPosHandle.AddrOfPinnedObject(),
                                                     camDirHandle.AddrOfPinnedObject(),
                                                     camUpHandle.AddrOfPinnedObject(),
                                                     camSideHandle.AddrOfPinnedObject(),
                                                     camNearFarFovHandle.AddrOfPinnedObject()); ;
        camPosHandle.Free();
        camDirHandle.Free();
        camUpHandle.Free();
        camSideHandle.Free();
        camNearFarFovHandle.Free();

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

        int camInstanceId = _camera.GetInstanceID();
        if (!_targets.ContainsKey(camInstanceId) || _targets[camInstanceId].width != width || _targets[camInstanceId].height != height)
        {
            _targets[camInstanceId] = new Texture2D(width, height, TextureFormat.RGBA32, false)
            {
                // Set point filtering just so we can see the pixels clearly
                filterMode = FilterMode.Point
            };

            // Call Apply() so it's actually uploaded to the GPU
            _targets[camInstanceId].Apply();

            if (PixelsForGlory.RayTracingPlugin.SetRenderTarget(camInstanceId, (int)_targets[camInstanceId].format, _targets[camInstanceId].width, _targets[camInstanceId].height, _targets[camInstanceId].GetNativeTexturePtr()) == 0)
            {
                Debug.Log("Something went wrong with setting render target");
                return false;
            }
        }

        return true;
    }

    private void RayTrace()
    {
        _commandBuffer.Clear();

        var cameraTextureId = new RenderTargetIdentifier(_camera.targetTexture);
        var cameraInstanceId = _camera.GetInstanceID();
        var cameraInstanceIdHandle = GCHandle.Alloc(cameraInstanceId, GCHandleType.Pinned);

        string sampleName = "Trace rays";
        _commandBuffer.BeginSample(sampleName);
        _commandBuffer.IssuePluginEventAndData(PixelsForGlory.RayTracingPlugin.GetEventAndDataFunc(), (int)Events.TraceRays, cameraInstanceIdHandle.AddrOfPinnedObject());
        _commandBuffer.Blit(_targets[_camera.GetInstanceID()], cameraTextureId);
        _commandBuffer.EndSample(sampleName);

        _context.ExecuteCommandBuffer(_commandBuffer);
        _context.Submit();

        _commandBuffer.Clear();

        cameraInstanceIdHandle.Free();
    }
}
