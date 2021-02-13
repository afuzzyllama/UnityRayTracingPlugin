using UnityEngine;

namespace PixelsForGlory.RayTracing
{
    [ExecuteInEditMode]
    [RequireComponent(typeof(Light))]
    class RayTraceableLight : MonoBehaviour
    {
        [ReadOnly] public int InstanceId;

        private Light _lightRef = null;

        private Light _light
        {
            get 
            {
                if(_lightRef == null)
                {
                    _lightRef = GetComponent<Light>();
                }
                return _lightRef;
            }
            
        }

        private bool _registeredWithRayTracer = false;

        private ValueMonitor _monitor = new ValueMonitor();

        private void OnEnable()
        {
            Initialize();
        }

        private void Initialize()
        {
            InstanceId = GetInstanceID();

            _monitor.AddProperty(transform, transform.GetType(), "position", transform.position);
            _monitor.AddProperty(_light, _light.GetType(), "color", _light.color);
            _monitor.AddProperty(_light, _light.GetType(), "bounceIntensity", _light.bounceIntensity);
            _monitor.AddProperty(_light, _light.GetType(), "intensity", _light.intensity);
            _monitor.AddProperty(_light, _light.GetType(), "range", _light.range);
            _monitor.AddProperty(_light, _light.GetType(), "spotAngle", _light.spotAngle);
            _monitor.AddProperty(_light, _light.GetType(), "type", _light.type);
            _monitor.AddProperty(_light, _light.GetType(), "enabled", _light.enabled);

            _registeredWithRayTracer = (PixelsForGlory.RayTracing.RayTracingPlugin.AddLight(InstanceId,
                                                                                            transform.position.x, transform.position.y, transform.position.z,
                                                                                            _light.color.r, _light.color.g, _light.color.b,
                                                                                            _light.bounceIntensity,
                                                                                            _light.intensity,
                                                                                            _light.range,
                                                                                            _light.spotAngle,
                                                                                            (int)_light.type,
                                                                                            _light.enabled) > 0);
        }

        private void Update()
        {
            if(InstanceId != GetInstanceID())
            {
                InstanceId = GetInstanceID();
            }

            UpdateLightData();
        }

        private void OnDisable()
        {
            UpdateLightData();
        }

        private void OnDestroy()
        {
            if(_registeredWithRayTracer)
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.RemoveLight(InstanceId);
            }       
        }

        private void UpdateLightData()
        {
            if(!_monitor.CheckForUpdates())
            {
                // Nothing to do
                return;
            }
            
            PixelsForGlory.RayTracing.RayTracingPlugin.UpdateLight(InstanceId,
                                                                   transform.position.x, transform.position.y, transform.position.z,
                                                                   _light.color.r, _light.color.g, _light.color.b,
                                                                   _light.bounceIntensity,
                                                                   _light.intensity,
                                                                   _light.range,
                                                                   _light.spotAngle,
                                                                   (int)_light.type,
                                                                   _light.enabled);
        }
    }
}
