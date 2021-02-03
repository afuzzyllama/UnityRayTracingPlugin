using System.Runtime.InteropServices;

using UnityEngine;

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

    private int _lastLightHash;

    private void OnEnable()
    {
        InstanceId = GetInstanceID();
        PixelsForGlory.RayTracingPlugin.AddLight(InstanceId,
                                                 transform.position.x, transform.position.y, transform.position.z,
                                                 _light.color.r, _light.color.g, _light.color.b,
                                                 _light.bounceIntensity,
                                                 _light.intensity,
                                                 _light.range,
                                                 _light.spotAngle,
                                                 (int)_light.type,
                                                 _light.enabled);
        UpdateLightData();
        _lastLightHash = _light.GetHashCode();
    }

    private void Update()
    {
        UpdateLightData();
    }

    private void OnDisable()
    {
        // Remove instance and possibly shared mesh
        UpdateLightData();
    }

    private void OnDestroy()
    {
        PixelsForGlory.RayTracingPlugin.RemoveLight(InstanceId);
    }

    private void UpdateLightData()
    {

        PixelsForGlory.RayTracingPlugin.UpdateLight(InstanceId,
                                                    transform.position.x, transform.position.y, transform.position.z,
                                                    _light.color.r, _light.color.g, _light.color.b,
                                                    _light.bounceIntensity,
                                                    _light.intensity,
                                                    _light.range,
                                                    _light.spotAngle,
                                                    (int)_light.type,
                                                    _light.enabled);
        _lastLightHash = _light.GetHashCode();
    }
}

