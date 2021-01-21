using System;
using System.Collections;
using System.Runtime.InteropServices;

using UnityEngine;

public class RayTracer : MonoBehaviour
{
    //IEnumerator Start()
    void Start()
    {
        var colorHandle = GCHandle.Alloc(RenderSettings.ambientSkyColor, GCHandleType.Pinned);
        PixelsForGlory.RayTracingPlugin.UpdateSceneData(colorHandle.AddrOfPinnedObject());
        colorHandle.Free();

        var camPosHandle        = GCHandle.Alloc(Camera.main.transform.position, GCHandleType.Pinned);
        var camDirHandle        = GCHandle.Alloc(Camera.main.transform.forward, GCHandleType.Pinned);
        var camUpHandle         = GCHandle.Alloc(Camera.main.transform.up, GCHandleType.Pinned);
        var camSideHandle       = GCHandle.Alloc(Camera.main.transform.right, GCHandleType.Pinned);
        var camNearFarFovHandle = GCHandle.Alloc(new Vector3(Camera.main.nearClipPlane, Camera.main.farClipPlane, Camera.main.fieldOfView), GCHandleType.Pinned);
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

        PixelsForGlory.RayTracingPlugin.BuildTlas();
        PixelsForGlory.RayTracingPlugin.Prepare(Screen.width, Screen.height);
        GL.IssuePluginEvent(PixelsForGlory.RayTracingPlugin.GetRenderEventFunc(), 1);
        //yield return StartCoroutine("CallPluginAtEndOfFrames");
    }

    class AccelerationStructureData
    {
        private GCHandle _vertices;
        private GCHandle _indices; 
        Matrix4x4 _localToWorld;
    }

    private IEnumerator CallPluginAtEndOfFrames()
    {
        while (true)
        {
            // Wait until all frame rendering is done
            yield return new WaitForEndOfFrame();

        //    // Set time for the plugin
        //    SetTimeFromUnity(Time.timeSinceLevelLoad);

        //    // Issue a plugin event with arbitrary integer identifier.
        //    // The plugin can distinguish between different
        //    // things it needs to do based on this ID.
        //    // For our simple plugin, it does not matter which ID we pass here.
        //    GL.IssuePluginEvent(GetRenderEventFunc(), 1);
        }
    }
}