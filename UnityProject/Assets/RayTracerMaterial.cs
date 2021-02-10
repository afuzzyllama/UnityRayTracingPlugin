using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[CreateAssetMenu(menuName = "Ray Tracing/Material")]
public class RayTracerMaterial : ScriptableObject
{
    public Texture2D albedoTexture;
    public Color     albedo;

    public Texture2D emissionTexture;
    public Color     emission; 

    public Texture2D normalTexture;

    public Texture2D                 metallicTexture;
    [Range(0.0f, 1.0f)] public float metallic;

    public Texture2D                 roughnessTexture;
    [Range(0.0f, 1.0f)] public float roughness;

    public Texture2D ambientOcclusionTexture;

    [Range(1.0003f, 5.0f)] public float indexOfRefraction;

    public Color transmittance;

    [ReadOnly] public int InstanceId;

    private ValueMonitor _monitor = new ValueMonitor();

    private void OnEnable()
    {
        Initialize();
    }

    private void Initialize()
    {
         InstanceId = GetInstanceID();
        _monitor.AddField(this, typeof(RayTracerMaterial), "albedoTexture", albedoTexture == null ? -1 : albedoTexture.GetInstanceID());
        _monitor.AddField(this, typeof(RayTracerMaterial), "albedo", albedo);
        _monitor.AddField(this, typeof(RayTracerMaterial), "emissionTexture", emissionTexture == null ? -1 : emissionTexture.GetInstanceID());
        _monitor.AddField(this, typeof(RayTracerMaterial), "emission", emission);
        _monitor.AddField(this, typeof(RayTracerMaterial), "normalTexture", normalTexture == null ? -1 : normalTexture.GetInstanceID());
        _monitor.AddField(this, typeof(RayTracerMaterial), "metallic", metallic);
        _monitor.AddField(this, typeof(RayTracerMaterial), "metallicTexture", metallicTexture == null ? -1 : metallicTexture.GetInstanceID());
        _monitor.AddField(this, typeof(RayTracerMaterial), "roughness", roughness);
        _monitor.AddField(this, typeof(RayTracerMaterial), "roughnessTexture", roughnessTexture == null ? -1 : roughnessTexture.GetInstanceID());
        _monitor.AddField(this, typeof(RayTracerMaterial), "ambientOcclusionTexture", ambientOcclusionTexture == null ? -1 : ambientOcclusionTexture.GetInstanceID());
        _monitor.AddField(this, typeof(RayTracerMaterial), "indexOfRefraction", indexOfRefraction);
        _monitor.AddField(this, typeof(RayTracerMaterial), "transmittance", transmittance);

        PixelsForGlory.RayTracingPlugin.AddMaterial(InstanceId,
                                                    albedo.r, albedo.g, albedo.b,
                                                    emission.r, emission.g, emission.b,
                                                    transmittance.r, transmittance.g, transmittance.b,
                                                    metallic,
                                                    roughness,
                                                    indexOfRefraction,
                                                   -1,
                                                   -1,
                                                   -1,
                                                   -1,
                                                   -1,
                                                   -1);
        //RayTracerMaterial.albedoTexture.GetInstanceID(),
        //RayTracerMaterial.emissionTexture.GetInstanceID(),
        //RayTracerMaterial.normalTexture.GetInstanceID(),
        //RayTracerMaterial.metallicTexture.GetInstanceID(),
        //RayTracerMaterial.roughnessTexture.GetInstanceID(),
        //RayTracerMaterial.ambientOcclusionTexture.GetInstanceID());
    }

    public void Update()
    {
        if(!_monitor.CheckForUpdates())
        {
            return;
        }

        PixelsForGlory.RayTracingPlugin.UpdateMaterial(InstanceId,
                                                       albedo.r, albedo.g, albedo.b,
                                                       emission.r, emission.g, emission.b,
                                                       transmittance.r, transmittance.g, transmittance.b,
                                                       metallic,
                                                       roughness,
                                                       indexOfRefraction,
                                                       -1,
                                                       -1,
                                                       -1,
                                                       -1,
                                                       -1,
                                                       -1);
        //RayTracerMaterial.albedoTexture.GetInstanceID(),
        //RayTracerMaterial.emissionTexture.GetInstanceID(),
        //RayTracerMaterial.normalTexture.GetInstanceID(),
        //RayTracerMaterial.metallicTexture.GetInstanceID(),
        //RayTracerMaterial.roughnessTexture.GetInstanceID(),
        //RayTracerMaterial.ambientOcclusionTexture.GetInstanceID());
    }
}
