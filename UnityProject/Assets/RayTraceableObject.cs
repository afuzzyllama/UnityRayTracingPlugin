using System.Runtime.InteropServices;

using UnityEngine;


[RequireComponent(typeof(MeshFilter))]
class RayTraceableObject : MonoBehaviour
{
    public int SharedMeshIndex;
    public int MeshInstanceIndex;

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
        SendMeshToPlugin();
        SendInstanceToPlugin();
    }

    private void OnDisable()
    {
        // Remove instance and possibly shared mesh
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

        var verticesHandle = GCHandle.Alloc(vertices, GCHandleType.Pinned);
        var normalsHandle = GCHandle.Alloc(normals, GCHandleType.Pinned);
        var uvsHandle = GCHandle.Alloc(uvs, GCHandleType.Pinned);
        var indicesHandle = GCHandle.Alloc(indices, GCHandleType.Pinned);

        SharedMeshIndex = PixelsForGlory.RayTracingPlugin.AddMesh(
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
        var l2wMatrix = transform.localToWorldMatrix;
        var l2wMatrixHandle = GCHandle.Alloc(l2wMatrix, GCHandleType.Pinned);
        
        MeshInstanceIndex = PixelsForGlory.RayTracingPlugin.AddTlasInstance(SharedMeshIndex, l2wMatrixHandle.AddrOfPinnedObject());

        l2wMatrixHandle.Free();
    }
}

