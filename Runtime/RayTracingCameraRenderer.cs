using UnityEngine;
using UnityEngine.Rendering;

using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace PixelsForGlory.RayTracing
{
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
            _context.SetupCameraProperties(_camera);
            
            var colorHandle = GCHandle.Alloc(_camera.backgroundColor, GCHandleType.Pinned);
            PixelsForGlory.RayTracing.RayTracingPlugin.UpdateSceneData(colorHandle.AddrOfPinnedObject());
            colorHandle.Free();

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

            int cameraInstanceId = _camera.GetInstanceID();
            if (!_targets.ContainsKey(cameraInstanceId) || _targets[cameraInstanceId].width != width || _targets[cameraInstanceId].height != height)
            {
                _targets[cameraInstanceId] = new Texture2D(width, height, TextureFormat.RGBA32, false)
                {
                    // Set point filtering just so we can see the pixels clearly
                    filterMode = FilterMode.Point
                };

                // Call Apply() so it's actually uploaded to the GPU
                _targets[cameraInstanceId].Apply();

                if (PixelsForGlory.RayTracing.RayTracingPlugin.SetRenderTarget(cameraInstanceId, (int)_targets[cameraInstanceId].format, _targets[cameraInstanceId].width, _targets[cameraInstanceId].height, _targets[cameraInstanceId].GetNativeTexturePtr()) == 0)
                {
                    Debug.Log("Something went wrong with setting render target");
                    return false;
                }
            }

            var camPosHandle = GCHandle.Alloc(_camera.transform.position, GCHandleType.Pinned);
            var camDirHandle = GCHandle.Alloc(_camera.transform.forward, GCHandleType.Pinned);
            var camUpHandle = GCHandle.Alloc(-_camera.transform.up, GCHandleType.Pinned);
            var camRightHandle = GCHandle.Alloc(_camera.transform.right, GCHandleType.Pinned);
            PixelsForGlory.RayTracing.RayTracingPlugin.UpdateCamera(cameraInstanceId,
                                                                    camPosHandle.AddrOfPinnedObject(),
                                                                    camDirHandle.AddrOfPinnedObject(),
                                                                    camUpHandle.AddrOfPinnedObject(),
                                                                    camRightHandle.AddrOfPinnedObject(),
                                                                    _camera.nearClipPlane,
                                                                    _camera.farClipPlane,
                                                                    Mathf.Deg2Rad * _camera.fieldOfView);
            camPosHandle.Free();
            camDirHandle.Free();
            camUpHandle.Free();
            camRightHandle.Free();

            return true;
        }

        private void RayTrace()
        {
            _commandBuffer.Clear();

            var cameraTextureId = new RenderTargetIdentifier(_camera.targetTexture);
            var cameraInstanceId = _camera.GetInstanceID();
            var cameraInstanceIdHandle = GCHandle.Alloc(cameraInstanceId, GCHandleType.Pinned);

    #if UNITY_EDITOR
            if (_camera.cameraType == CameraType.SceneView)
            {
                ScriptableRenderContext.EmitWorldGeometryForSceneView(_camera);
            }
    #endif

            string sampleName = "Trace rays";
            _commandBuffer.BeginSample(sampleName);
            _commandBuffer.IssuePluginEventAndData(PixelsForGlory.RayTracing.RayTracingPlugin.GetEventAndDataFunc(), (int)Events.TraceRays, cameraInstanceIdHandle.AddrOfPinnedObject());
            _commandBuffer.Blit(_targets[_camera.GetInstanceID()], cameraTextureId);
            _commandBuffer.EndSample(sampleName);

            _context.ExecuteCommandBuffer(_commandBuffer);

    #if UNITY_EDITOR
            if (_camera.cameraType == CameraType.SceneView)
            {
                _context.DrawGizmos(_camera, GizmoSubset.PreImageEffects);
                _context.DrawGizmos(_camera, GizmoSubset.PostImageEffects);
            }
    #endif
            _context.Submit();

            _commandBuffer.Clear();

            cameraInstanceIdHandle.Free();
        }
    }
}