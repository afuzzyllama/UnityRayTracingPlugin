using System.Runtime.InteropServices;

using UnityEngine;

[ExecuteInEditMode]
[RequireComponent(typeof(MeshFilter))]
class RayTraceableObject : MonoBehaviour
{
    [ReadOnly] public int SharedMeshInstanceId;
    [ReadOnly] public int SharedMeshIndex;
    [ReadOnly] public int MeshInstanceIndex;

    private MeshFilter _meshFilterRef = null;
    private MeshFilter _meshFilter
    {
        get 
        {
            if(_meshFilterRef == null)
            {
                _meshFilterRef = GetComponent<MeshFilter>();
            }
            return _meshFilterRef;
        }
        
    }

    private void OnEnable()
    {
        SharedMeshInstanceId = _meshFilter.sharedMesh.GetInstanceID();
        SendMeshToPlugin();
        SendInstanceToPlugin();
    }

    private void OnDisable()
    {
        // Remove instance and possibly shared mesh
        RemoveInstanceFromPlugin();
    }
    private void SendMeshToPlugin()
    {
        SharedMeshIndex = PixelsForGlory.RayTracingPlugin.GetSharedMeshIndex(_meshFilter.sharedMesh.GetInstanceID());
        
        if(SharedMeshIndex >= 0)
        { 
            // Nothing to do
            return;
        }

        var vertices = _meshFilter.sharedMesh.vertices;
        var normals = _meshFilter.sharedMesh.normals;
        var uvs = _meshFilter.sharedMesh.uv;
        var indices = _meshFilter.sharedMesh.triangles;

        //Debug.Log($"{_meshFilter.sharedMesh.GetInstanceID()} verts: {vertices.Length}");
        //Debug.Log($"{_meshFilter.sharedMesh.GetInstanceID()} indices: {indices.Length}");

        var verticesHandle = GCHandle.Alloc(vertices, GCHandleType.Pinned);
        var normalsHandle = GCHandle.Alloc(normals, GCHandleType.Pinned);
        var uvsHandle = GCHandle.Alloc(uvs, GCHandleType.Pinned);
        var indicesHandle = GCHandle.Alloc(indices, GCHandleType.Pinned);

        SharedMeshIndex = PixelsForGlory.RayTracingPlugin.AddSharedMesh(
                                            _meshFilter.sharedMesh.GetInstanceID(),
                                            verticesHandle.AddrOfPinnedObject(),
                                            normalsHandle.AddrOfPinnedObject(),
                                            uvsHandle.AddrOfPinnedObject(),
                                            vertices.Length,
                                            indicesHandle.AddrOfPinnedObject(),
                                            indices.Length);

        verticesHandle.Free();
        normalsHandle.Free();
        uvsHandle.Free();
        indicesHandle.Free();
    }

    private void SendInstanceToPlugin()
    {
        MeshInstanceIndex = PixelsForGlory.RayTracingPlugin.GetTlasInstanceIndex(GetInstanceID());

        if (MeshInstanceIndex >= 0)
        {
            // Nothing to do
            return;
        }

        // Send instance information to plugin ONCE
        

        var l2wMatrix = transform.localToWorldMatrix;
        var l2wMatrixHandle = GCHandle.Alloc(l2wMatrix, GCHandleType.Pinned);

        //Debug.Log("SendInstanceToPlugin");
        //Debug.Log($"{l2wMatrix[0,0]} {l2wMatrix[0,1]} {l2wMatrix[0,2]} {l2wMatrix[0,3]}");
        //Debug.Log($"{l2wMatrix[1,0]} {l2wMatrix[1,1]} {l2wMatrix[1,2]} {l2wMatrix[1,3]}");
        //Debug.Log($"{l2wMatrix[2,0]} {l2wMatrix[2,1]} {l2wMatrix[2,2]} {l2wMatrix[2,3]}");
        //Debug.Log($"{l2wMatrix[3,0]} {l2wMatrix[3,1]} {l2wMatrix[3,2]} {l2wMatrix[3,3]}");
        //Debug.Log("--------------------");

        MeshInstanceIndex = PixelsForGlory.RayTracingPlugin.AddTlasInstance(GetInstanceID(), SharedMeshIndex, l2wMatrixHandle.AddrOfPinnedObject());

        l2wMatrixHandle.Free();
    }

    private void RemoveInstanceFromPlugin()
    {
        PixelsForGlory.RayTracingPlugin.RemoveTlasInstance(MeshInstanceIndex);
    }
}

