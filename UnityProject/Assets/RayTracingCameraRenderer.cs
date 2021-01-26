using UnityEngine;
using UnityEngine.Rendering;

using System.Collections.Generic;

public class RayTracingCameraRenderer
{
    ScriptableRenderContext _context;
    Camera _camera;

    // Camera.GetInstanceId() => Target Texture2D. Hold one for each active camera
    // TOOD: How to remove camera that is no longer active?
    Dictionary<CameraType, Texture2D> _targets = new Dictionary<CameraType, Texture2D>();

    public void Render(ScriptableRenderContext context, Camera camera)
    {
        _context = context;
        _camera = camera;

        if(Setup())
        {
            RayTrace();
            Submit();
        }

    }

    private bool Setup()
    {
        _context.SetupCameraProperties(_camera);

        // If this camera type hasn't been handled, make an entry
        if (!_targets.ContainsKey(_camera.cameraType))
        { 
            _targets[_camera.cameraType] = null;
        }

        if(_camera.cameraType != CameraType.Game && _camera.cameraType != CameraType.SceneView)
        {
            return false;
        }

        // TODO: This is actually pretty terrible for editor performance with mulitple scene views.
        //       If there are multiple scene views of different sizes, it will recreate the target texture every frame for the editor
        if(_targets[_camera.cameraType] == null || RebuildTargetTexture())
        {
            int newWidth = 0;
            int newHeight = 0;
            // Get a render target for Ray Tracing
            if (_camera.cameraType == CameraType.SceneView)
            {
                newWidth = _camera.targetTexture.width;
                newHeight = _camera.targetTexture.height;
            }
            else if (_camera.cameraType == CameraType.Game)
            {
                newWidth = Screen.width;
                newHeight = Screen.height;
            }

            _targets[_camera.cameraType] = new Texture2D(newWidth, newHeight, TextureFormat.RGBA32, false)
            {
                // Set point filtering just so we can see the pixels clearly
                filterMode = FilterMode.Point
            };

            // Call Apply() so it's actually uploaded to the GPU
            _targets[_camera.cameraType].Apply();

            if(PixelsForGlory.RayTracingPlugin.SetRenderTarget((int)_camera.cameraType,
                                                                   (int)_targets[_camera.cameraType].format,
                                                                   _targets[_camera.cameraType].width,
                                                                   _targets[_camera.cameraType].height,
                                                                   _targets[_camera.cameraType].GetNativeTexturePtr()) == 0)
            {
                // Set this to null to ensure it will fail again next pass
                _targets[_camera.cameraType] = null;
            }

        }

        
        // If we got here, we are successful
        return true;
    }

    private bool RebuildTargetTexture()
    {
        if(_camera.cameraType == CameraType.SceneView)
        {
            if(_targets[_camera.cameraType].width != _camera.targetTexture.width || _targets[_camera.cameraType].height != _camera.targetTexture.height)
            {
                return true;
            }
        }
        else if(_camera.cameraType == CameraType.Game)
        {
            if (_targets[_camera.cameraType].width != Screen.width || _targets[_camera.cameraType].height != Screen.height)
            {
                return true;
            }
        }

        return false;
    }

    private void RayTrace()
    {
        // Issue the ray trace and execute the command buffer right away
        GL.IssuePluginEvent(PixelsForGlory.RayTracingPlugin.GetRenderEventFunc(), (int)_camera.cameraType);
    }

    private void Submit()
    {
        Graphics.Blit(_targets[_camera.cameraType], _camera.targetTexture);
    }
}
