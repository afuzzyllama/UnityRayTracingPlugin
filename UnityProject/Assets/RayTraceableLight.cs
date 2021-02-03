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

    private Vector3     _lastPosition;
    private Color       _lastColor;
    private float       _lastBounceIntensity;
    private float       _lastIintensity;
    private float       _lastRange;
    private float       _lastSpotAngle;
    private LightType   _lastType;
    private bool        _lastEnabled;

    private bool _registeredWithRayTracer = false;

    private void OnEnable()
    {
        InstanceId = GetInstanceID();
        _registeredWithRayTracer = (PixelsForGlory.RayTracingPlugin.AddLight(InstanceId,
                                                 transform.position.x, transform.position.y, transform.position.z,
                                                 _light.color.r, _light.color.g, _light.color.b,
                                                 _light.bounceIntensity,
                                                 _light.intensity,
                                                 _light.range,
                                                 _light.spotAngle,
                                                 (int)_light.type,
                                                 _light.enabled) > 0);

        _lastPosition = transform.position;
        _lastColor = _light.color;
        _lastBounceIntensity = _light.bounceIntensity;
        _lastIintensity = _light.intensity;
        _lastRange = _light.range;
        _lastSpotAngle = _light.spotAngle;
        _lastType = _light.type;
        _lastEnabled = _light.enabled;
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
        if(_registeredWithRayTracer)
        {
            PixelsForGlory.RayTracingPlugin.RemoveLight(InstanceId);
        }
        
    }

    private void UpdateLightData()
    {
        if(_registeredWithRayTracer &&
            (
                _lastPosition        != transform.position       ||
                _lastColor           != _light.color             ||
                _lastBounceIntensity != _light.bounceIntensity   ||
                _lastIintensity      !=_light.intensity          ||
                _lastRange           != _light.range             ||
                _lastSpotAngle       != _light.spotAngle         ||
                _lastType            != _light.type              ||
                _lastEnabled         != _light.enabled
            )
        )
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

            _lastPosition           = transform.position;
            _lastColor              = _light.color;
            _lastBounceIntensity    = _light.bounceIntensity;
            _lastIintensity         = _light.intensity;
            _lastRange              = _light.range;
            _lastSpotAngle          = _light.spotAngle;
            _lastType               = _light.type;
            _lastEnabled            = _light.enabled;
        }
    }
}

