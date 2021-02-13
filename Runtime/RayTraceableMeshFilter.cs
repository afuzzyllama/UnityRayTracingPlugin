using UnityEngine;

using System.Runtime.InteropServices;

namespace PixelsForGlory.RayTracing
{
    [ExecuteInEditMode]
    [RequireComponent(typeof(MeshFilter))]
    class RayTraceableMeshFilter : MonoBehaviour
    {
        [ReadOnly] public int SharedMeshInstanceId;
        [ReadOnly] public bool SharedMeshRegisteredWithRayTracer = false;

        private MeshFilter _meshFilterRef = null;
        private MeshFilter _meshFilter
        {
            get
            {
                if (_meshFilterRef == null)
                {
                    _meshFilterRef = GetComponent<MeshFilter>();
                }
                return _meshFilterRef;
            }
        }

        private ValueMonitor _monitor = new ValueMonitor();

        private void OnEnable()
        {
            Initialize();
        }

        private void Initialize()
        {
            SharedMeshInstanceId = _meshFilter.sharedMesh.GetInstanceID();
            AddMeshData();
            _monitor.AddProperty(_meshFilter, _meshFilter.GetType(), "sharedMesh", _meshFilter.sharedMesh);
        }

        private void AddMeshData()
        {
            // Make sure we got these calculated!
            _meshFilter.sharedMesh.RecalculateNormals();
            _meshFilter.sharedMesh.RecalculateBounds();
            _meshFilter.sharedMesh.RecalculateTangents();

            var vertices = _meshFilter.sharedMesh.vertices;
            var normals = _meshFilter.sharedMesh.normals;
            var tangents = _meshFilter.sharedMesh.tangents;
            var uvs = _meshFilter.sharedMesh.uv;
            var indices = _meshFilter.sharedMesh.triangles;

            //Debug.Log($"{_meshFilter.sharedMesh.GetInstanceID()} verts: {vertices.Length}");
            //Debug.Log($"{_meshFilter.sharedMesh.GetInstanceID()} indices: {indices.Length}");

            var verticesHandle = GCHandle.Alloc(vertices, GCHandleType.Pinned);
            var normalsHandle = GCHandle.Alloc(normals, GCHandleType.Pinned);
            var tangetsHandle = GCHandle.Alloc(tangents, GCHandleType.Pinned);
            var uvsHandle = GCHandle.Alloc(uvs, GCHandleType.Pinned);
            var indicesHandle = GCHandle.Alloc(indices, GCHandleType.Pinned);

            SharedMeshRegisteredWithRayTracer = (PixelsForGlory.RayTracing.RayTracingPlugin.AddSharedMesh(SharedMeshInstanceId,
                                                                                                          verticesHandle.AddrOfPinnedObject(),
                                                                                                          normalsHandle.AddrOfPinnedObject(),
                                                                                                          tangetsHandle.AddrOfPinnedObject(),
                                                                                                          uvsHandle.AddrOfPinnedObject(),
                                                                                                          vertices.Length,
                                                                                                          indicesHandle.AddrOfPinnedObject(),
                                                                                                          indices.Length) > 0);

            verticesHandle.Free();
            normalsHandle.Free();
            tangetsHandle.Free();
            uvsHandle.Free();
            indicesHandle.Free();
        }
    }
}