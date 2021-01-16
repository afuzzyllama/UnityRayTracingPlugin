using UnityEditor;
using UnityEngine;

using Unity.Collections;
using Unity.Collections.LowLevel.Unsafe;

using System;
using System.Collections;
using System.Runtime.InteropServices;
using AOT;
using System.Collections.Generic;

#if UNTIY_EDITOR
[InitializeOnLoad]
#endif
public class RayTracer : MonoBehaviour
{
#if UNTIY_EDITOR
    static UseRenderingPlugin()
    { 
        RegisterDebugCallback(OnDebugCallback);
    }
#endif

    // Native plugin rendering events are only called if a plugin is used
    // by some script. This means we have to DllImport at least
    // one function in some active script.
    // For this example, we'll call into plugin's SetTimeFromUnity
    // function and pass the current time so the plugin can animate.


    [DllImport("RayTracingPlugin")]
    private static extern void SetTimeFromUnity(float t);


    // We'll also pass native pointer to a texture in Unity.
    // The plugin will fill texture data from native code.
    //[DllImport("RenderingPlugin")]
    //private static extern void SetTextureFromUnity(System.IntPtr texture, int w, int h);

    // We'll pass native pointer to the mesh vertex buffer.
    // Also passing source unmodified mesh data.
    // The plugin will fill vertex data from native code.
    //[DllImport("RenderingPlugin")]
    //private static extern void SetMeshBuffersFromUnity(IntPtr vertexBuffer, int vertexCount, IntPtr sourceVertices, IntPtr sourceNormals, IntPtr sourceUVs);
    [DllImport("RayTracingPlugin")]
    private static extern void AddMesh(int sharedMeshInstanceId, IntPtr vertices, IntPtr normals, IntPtr uvs, int vertexCount, IntPtr indices, int indexCount);

    [DllImport("RayTracingPlugin")]
    private static extern void AddTlasInstance(int meshInstanceId, int sharedMeshInstanceId, IntPtr l2wMatrix);

    [DllImport("RayTracingPlugin")]
    private static extern void BuildTlas();


    [DllImport("RayTracingPlugin")]
    private static extern IntPtr GetRenderEventFunc();

    [DllImport("RayTracingPlugin", CallingConvention = CallingConvention.Cdecl)]
    private static extern void RegisterDebugCallback(DebugCallback callback);

    //Create string param callback delegate
    delegate void DebugCallback(IntPtr request, int level, int size);

    [MonoPInvokeCallback(typeof(DebugCallback))]
    static void OnDebugCallback(IntPtr request, int level, int size)
    {
        //Ptr to string
        string debug_string = Marshal.PtrToStringAnsi(request, size);

        switch (level)
        {
            case 0:
                Debug.Log(debug_string);
                break;

            case 1:
                Debug.LogWarning(debug_string);
                break;

            case 2:
                Debug.LogError(debug_string);
                break;

            default:
                Debug.LogError($"Unsupported log level {level}");
                break;
        }
    }

    private void OnEnable()
    {
#if !UNTIY_EDITOR
        RegisterDebugCallback(OnDebugCallback);
#endif

    }

    //IEnumerator Start()
    void Start()
    {
        SendMeshesToPlugin();
        SendInstanceToPlugin();
        BuildTlas();
        //CreateTextureAndPassToPlugin();
        //SendMeshBuffersToPlugin();
        //yield return StartCoroutine("CallPluginAtEndOfFrames");
    }

    class AccelerationStructureData
    {
        private GCHandle _vertices;
        private GCHandle _indices; 
        Matrix4x4 _localToWorld;
    }

    private void SendMeshesToPlugin()
    {
        var foundMeshFilters = FindObjectsOfType<MeshFilter>();

        foreach (var meshFilter in foundMeshFilters)
        {
            var vertices = meshFilter.sharedMesh.vertices;
            var normals = meshFilter.sharedMesh.normals;
            var uvs = meshFilter.sharedMesh.uv;
            var indices = meshFilter.sharedMesh.triangles;

            var verticesHandle = GCHandle.Alloc(vertices, GCHandleType.Pinned);
            var normalsHandle = GCHandle.Alloc(normals, GCHandleType.Pinned);
            var uvsHandle = GCHandle.Alloc(uvs, GCHandleType.Pinned);
            var indicesHandle = GCHandle.Alloc(indices, GCHandleType.Pinned);

            AddMesh(meshFilter.sharedMesh.GetInstanceID(), verticesHandle.AddrOfPinnedObject(), normalsHandle.AddrOfPinnedObject(), uvsHandle.AddrOfPinnedObject(), vertices.Length, indicesHandle.AddrOfPinnedObject(), indices.Length);

            verticesHandle.Free();
            normalsHandle.Free();
            uvsHandle.Free();
            indicesHandle.Free();
        }
    }

    private void SendInstanceToPlugin()
    {
        var foundMeshFilters = FindObjectsOfType<MeshFilter>();

        // Get instance transforms for tlas
        for (int i = 0; i < foundMeshFilters.Length; ++i)
        {
            var meshFilter = foundMeshFilters[i];

            var l2wMatrix = new float[16] {
                meshFilter.transform.localToWorldMatrix.m00,
                meshFilter.transform.localToWorldMatrix.m01,
                meshFilter.transform.localToWorldMatrix.m02,
                meshFilter.transform.localToWorldMatrix.m03,
                meshFilter.transform.localToWorldMatrix.m10,
                meshFilter.transform.localToWorldMatrix.m11,
                meshFilter.transform.localToWorldMatrix.m12,
                meshFilter.transform.localToWorldMatrix.m13,
                meshFilter.transform.localToWorldMatrix.m20,
                meshFilter.transform.localToWorldMatrix.m21,
                meshFilter.transform.localToWorldMatrix.m22,
                meshFilter.transform.localToWorldMatrix.m23,
                meshFilter.transform.localToWorldMatrix.m30,
                meshFilter.transform.localToWorldMatrix.m31,
                meshFilter.transform.localToWorldMatrix.m32,
                meshFilter.transform.localToWorldMatrix.m33
            };

            var l2wMatrixHandle = GCHandle.Alloc(l2wMatrix, GCHandleType.Pinned);

            AddTlasInstance(meshFilter.GetInstanceID(), meshFilter.sharedMesh.GetInstanceID(), l2wMatrixHandle.AddrOfPinnedObject());

            l2wMatrixHandle.Free();
        }
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