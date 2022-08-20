using UnityEngine;

using System.Runtime.InteropServices;
using System;

namespace PixelsForGlory.RayTracing
{
    [ExecuteInEditMode]
    [RequireComponent(typeof(MeshFilter))]
    public class RayTraceableMeshFilter : MonoBehaviour
    {
        [ReadOnly] public int SharedMeshInstanceId;

        [NonSerialized]
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

        public void Reinitialize()
        {
            Initialize();
        }

        private void OnEnable()
        {
            Initialize();
        }

        private void Update()
        {
            if(!SharedMeshRegisteredWithRayTracer && _meshFilter.sharedMesh != null)
            {
                SharedMeshInstanceId = _meshFilter.sharedMesh.GetInstanceID();
                AddMeshData();
                _monitor.AddProperty(_meshFilter, _meshFilter.GetType(), "sharedMesh", _meshFilter.sharedMesh);
            }
        }

        private void Initialize()
        {
            if(_meshFilter.sharedMesh != null)
            {
                SharedMeshInstanceId = _meshFilter.sharedMesh.GetInstanceID();
                AddMeshData();
                _monitor.AddProperty(_meshFilter, _meshFilter.GetType(), "sharedMesh", _meshFilter.sharedMesh);
            }
        }

        internal void UnregisterSharedMesh()
        {
            RemoveMeshData();
            _meshFilter.sharedMesh = null;
            SharedMeshRegisteredWithRayTracer = false;
        }

        internal void UpdateSharedMesh(Mesh mesh)
        {
            // Remove the old shared mesh if applicable 
            UnregisterSharedMesh();

            // This will cause mesh to update on next update call
            _meshFilter.sharedMesh = mesh;
            SharedMeshInstanceId = _meshFilter.sharedMesh.GetInstanceID();
        }

        private void AddMeshData()
        {
            if(SharedMeshRegisteredWithRayTracer == true)
            {
                // Already added
                return;
            }

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

        private void RemoveMeshData()
        {
            if(!SharedMeshRegisteredWithRayTracer)
            {
                // Not registered, nothing to do
                return;
            }

            // This might not remove the shared mesh, but if this is the only filter that references it, it will be removed
            PixelsForGlory.RayTracing.RayTracingPlugin.RemoveSharedMesh(SharedMeshInstanceId);
        }
    }
}