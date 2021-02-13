using System.Runtime.InteropServices;

using UnityEngine;

namespace PixelsForGlory.RayTracing
{
    [ExecuteInEditMode]
    [RequireComponent(typeof(RayTraceableMeshFilter))]
    class RayTraceableObject : MonoBehaviour
    {
        [ReadOnly] public int InstanceId;
        [ReadOnly] public int MaterialInstanceId;

        public RayTracerMaterial RayTracerMaterial;

        private RayTraceableMeshFilter _rayTracableMeshFilterRef;
        private RayTraceableMeshFilter _rayTracableMeshFilter
        {
            get
            {
                if (_rayTracableMeshFilterRef == null)
                {
                    _rayTracableMeshFilterRef = GetComponent<RayTraceableMeshFilter>();
                }

                return _rayTracableMeshFilterRef;
            }
        }
        
        private bool _meshInstanceRegisteredWithRayTracer = false;
        private ValueMonitor _monitor = new ValueMonitor();

        private void OnEnable()
        {
            Initialize();
        }

        private void Initialize()
        {
            InstanceId = GetInstanceID();
            _monitor.AddProperty(transform, transform.GetType(), "localToWorldMatrix", transform.localToWorldMatrix);
            _monitor.AddProperty(transform, transform.GetType(), "worldToLocalMatrix", transform.worldToLocalMatrix);
        }

        private void Update()
        {
            if(_meshInstanceRegisteredWithRayTracer)
            {
                UpdateInstance();
                if(RayTracerMaterial != null)
                {
                    RayTracerMaterial.Update();
                }
                
            }
            else if(_rayTracableMeshFilter.SharedMeshRegisteredWithRayTracer && !_meshInstanceRegisteredWithRayTracer)
            {
                AddInstanceToPlugin();
            }
        }

        private void OnDisable()
        {
            // Remove instance and possibly shared mesh
            RemoveInstanceFromPlugin();
        }

        private void AddInstanceToPlugin()
        {
            
            var l2wMatrix = transform.localToWorldMatrix;
            var w2lMatrix = transform.worldToLocalMatrix;
            var l2wMatrixHandle = GCHandle.Alloc(l2wMatrix, GCHandleType.Pinned);
            var w2lMatrixHandle = GCHandle.Alloc(w2lMatrix, GCHandleType.Pinned);

            //Debug.Log("SendInstanceToPlugin");
            //Debug.Log($"{l2wMatrix[0,0]} {l2wMatrix[0,1]} {l2wMatrix[0,2]} {l2wMatrix[0,3]}");
            //Debug.Log($"{l2wMatrix[1,0]} {l2wMatrix[1,1]} {l2wMatrix[1,2]} {l2wMatrix[1,3]}");
            //Debug.Log($"{l2wMatrix[2,0]} {l2wMatrix[2,1]} {l2wMatrix[2,2]} {l2wMatrix[2,3]}");
            //Debug.Log($"{l2wMatrix[3,0]} {l2wMatrix[3,1]} {l2wMatrix[3,2]} {l2wMatrix[3,3]}");
            //Debug.Log("--------------------");

            MaterialInstanceId = RayTracerMaterial == null ? -1 : RayTracerMaterial.InstanceId;

            _meshInstanceRegisteredWithRayTracer = (PixelsForGlory.RayTracing.RayTracingPlugin.AddTlasInstance(InstanceId, 
                                                                                                               _rayTracableMeshFilter.SharedMeshInstanceId,
                                                                                                               MaterialInstanceId, 
                                                                                                               l2wMatrixHandle.AddrOfPinnedObject(),
                                                                                                               w2lMatrixHandle.AddrOfPinnedObject()) > 0);

            l2wMatrixHandle.Free();
            w2lMatrixHandle.Free();
        }

        private void UpdateInstance()
        {
            if(!_monitor.CheckForUpdates())
            {
                return;
            }

            MaterialInstanceId = RayTracerMaterial == null ? -1 : RayTracerMaterial.InstanceId;

            // Only update tlas instance if the transform has changed
            var l2wMatrix = transform.localToWorldMatrix;
            var w2lMatrix = transform.worldToLocalMatrix;
            var l2wMatrixHandle = GCHandle.Alloc(l2wMatrix, GCHandleType.Pinned);
            var w2lMatrixHandle = GCHandle.Alloc(w2lMatrix, GCHandleType.Pinned);

            PixelsForGlory.RayTracing.RayTracingPlugin.UpdateTlasInstance(InstanceId,
                                                                          MaterialInstanceId,
                                                                          l2wMatrixHandle.AddrOfPinnedObject(),
                                                                          w2lMatrixHandle.AddrOfPinnedObject());





            l2wMatrixHandle.Free();
            w2lMatrixHandle.Free();
        }

        private void RemoveInstanceFromPlugin()
        {
            if (_meshInstanceRegisteredWithRayTracer)
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.RemoveTlasInstance(InstanceId);
                _meshInstanceRegisteredWithRayTracer = false;
            }
        }
    }
}
