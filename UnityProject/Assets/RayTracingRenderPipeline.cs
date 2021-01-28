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
            var camPosHandle = GCHandle.Alloc(camera.transform.position, GCHandleType.Pinned);
            var camDirHandle = GCHandle.Alloc(camera.transform.forward, GCHandleType.Pinned);
            var camUpHandle = GCHandle.Alloc(camera.transform.up, GCHandleType.Pinned);
            var camSideHandle = GCHandle.Alloc(camera.transform.right, GCHandleType.Pinned);
            var camNearFarFovHandle = GCHandle.Alloc(new Vector3(camera.nearClipPlane, camera.farClipPlane, camera.fieldOfView), GCHandleType.Pinned);
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

            renderer.Render(context, camera);
        }

    }
}
