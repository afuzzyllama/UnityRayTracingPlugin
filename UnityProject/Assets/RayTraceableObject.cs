using System.Runtime.InteropServices;

using UnityEngine;

[ExecuteInEditMode]
[RequireComponent(typeof(MeshFilter))]
class RayTraceableObject : MonoBehaviour
{
    [ReadOnly] public int InstanceId;
    [ReadOnly] public int SharedMeshInstanceId;

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
        InstanceId = GetInstanceID();
        SharedMeshInstanceId = _meshFilter.sharedMesh.GetInstanceID();
        SendMeshToPlugin();
        SendInstanceToPlugin();
    }

    private void Update()
    {
        if(transform.hasChanged)
        {
            var l2wMatrix = transform.localToWorldMatrix;
            var l2wMatrixHandle = GCHandle.Alloc(l2wMatrix, GCHandleType.Pinned);
            PixelsForGlory.RayTracingPlugin.UpdateTlasInstance(InstanceId, l2wMatrixHandle.AddrOfPinnedObject());
            l2wMatrixHandle.Free();
        }
    }

    private void OnDisable()
    {
        // Remove instance and possibly shared mesh
        RemoveInstanceFromPlugin();
    }
    private void SendMeshToPlugin()
    {
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

        PixelsForGlory.RayTracingPlugin.AddSharedMesh(SharedMeshInstanceId,
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
        var l2wMatrix = transform.localToWorldMatrix;
        var l2wMatrixHandle = GCHandle.Alloc(l2wMatrix, GCHandleType.Pinned);

        //Debug.Log("SendInstanceToPlugin");
        //Debug.Log($"{l2wMatrix[0,0]} {l2wMatrix[0,1]} {l2wMatrix[0,2]} {l2wMatrix[0,3]}");
        //Debug.Log($"{l2wMatrix[1,0]} {l2wMatrix[1,1]} {l2wMatrix[1,2]} {l2wMatrix[1,3]}");
        //Debug.Log($"{l2wMatrix[2,0]} {l2wMatrix[2,1]} {l2wMatrix[2,2]} {l2wMatrix[2,3]}");
        //Debug.Log($"{l2wMatrix[3,0]} {l2wMatrix[3,1]} {l2wMatrix[3,2]} {l2wMatrix[3,3]}");
        //Debug.Log("--------------------");

        PixelsForGlory.RayTracingPlugin.AddTlasInstance(InstanceId, SharedMeshInstanceId, l2wMatrixHandle.AddrOfPinnedObject());

        l2wMatrixHandle.Free();
    }

    private void RemoveInstanceFromPlugin()
    {
        PixelsForGlory.RayTracingPlugin.RemoveTlasInstance(InstanceId);
    }
}

