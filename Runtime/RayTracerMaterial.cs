using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace PixelsForGlory.RayTracing
{
    [CreateAssetMenu(menuName = "Pixels for Glory/Ray Tracing/Material")]
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

        private ValueMonitor _materialMonitor = new ValueMonitor();
        private ValueMonitor _albedoTextureMonitor = new ValueMonitor();
        private ValueMonitor _emissionTextureMonitor = new ValueMonitor();
        private ValueMonitor _normalTextureMonitor = new ValueMonitor();
        private ValueMonitor _metallicTextureMonitor = new ValueMonitor();
        private ValueMonitor _roughnessTextureMonitor = new ValueMonitor();
        private ValueMonitor _ambientOcclusionTextureMonitor = new ValueMonitor();

        private void OnEnable()
        {
            Initialize();
        }

        private void Initialize()
        {
            InstanceId = GetInstanceID();

            _albedoTextureMonitor.AddField(this, typeof(RayTracerMaterial), "albedoTexture", albedoTexture);
            _emissionTextureMonitor.AddField(this, typeof(RayTracerMaterial), "emissionTexture", albedoTexture);
            _normalTextureMonitor.AddField(this, typeof(RayTracerMaterial), "normalTexture", albedoTexture);
            _metallicTextureMonitor.AddField(this, typeof(RayTracerMaterial), "metallicTexture", albedoTexture);
            _roughnessTextureMonitor.AddField(this, typeof(RayTracerMaterial), "roughnessTexture", albedoTexture);
            _ambientOcclusionTextureMonitor.AddField(this, typeof(RayTracerMaterial), "ambientOcclusionTexture", albedoTexture);

            _materialMonitor.AddField(this, typeof(RayTracerMaterial), "albedoTexture", albedoTexture);
            _materialMonitor.AddField(this, typeof(RayTracerMaterial), "albedo", albedo);
            _materialMonitor.AddField(this, typeof(RayTracerMaterial), "emissionTexture", emissionTexture);
            _materialMonitor.AddField(this, typeof(RayTracerMaterial), "emission", emission);
            _materialMonitor.AddField(this, typeof(RayTracerMaterial), "normalTexture", normalTexture);
            _materialMonitor.AddField(this, typeof(RayTracerMaterial), "metallic", metallic);
            _materialMonitor.AddField(this, typeof(RayTracerMaterial), "metallicTexture", metallicTexture);
            _materialMonitor.AddField(this, typeof(RayTracerMaterial), "roughness", roughness);
            _materialMonitor.AddField(this, typeof(RayTracerMaterial), "roughnessTexture", roughnessTexture);
            _materialMonitor.AddField(this, typeof(RayTracerMaterial), "ambientOcclusionTexture", ambientOcclusionTexture);
            _materialMonitor.AddField(this, typeof(RayTracerMaterial), "indexOfRefraction", indexOfRefraction);
            _materialMonitor.AddField(this, typeof(RayTracerMaterial), "transmittance", transmittance);

            if(albedoTexture != null)
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.AddTexture(albedoTexture.GetInstanceID(), albedoTexture.GetNativeTexturePtr());
            }

            if (emissionTexture != null)
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.AddTexture(emissionTexture.GetInstanceID(), emissionTexture.GetNativeTexturePtr());
            }

            if (normalTexture != null)
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.AddTexture(normalTexture.GetInstanceID(), normalTexture.GetNativeTexturePtr());
            }

            if (metallicTexture != null)
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.AddTexture(metallicTexture.GetInstanceID(), metallicTexture.GetNativeTexturePtr());
            }

            if (roughnessTexture != null)
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.AddTexture(roughnessTexture.GetInstanceID(), roughnessTexture.GetNativeTexturePtr());
            }

            if (ambientOcclusionTexture != null)
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.AddTexture(ambientOcclusionTexture.GetInstanceID(), ambientOcclusionTexture.GetNativeTexturePtr());
            }

            PixelsForGlory.RayTracing.RayTracingPlugin.AddMaterial(InstanceId,
                                                                   albedo.r, albedo.g, albedo.b,
                                                                   emission.r, emission.g, emission.b,
                                                                   transmittance.r, transmittance.g, transmittance.b,
                                                                   metallic,
                                                                   roughness,
                                                                   indexOfRefraction,
                                                                   albedoTexture != null ? true : false,
                                                                   albedoTexture != null ? albedoTexture.GetInstanceID() : -1,
                                                                   emissionTexture != null ? true : false,
                                                                   emissionTexture != null ? emissionTexture.GetInstanceID() : -1,
                                                                   normalTexture != null ? true : false,
                                                                   normalTexture != null ? normalTexture.GetInstanceID() : -1,
                                                                   metallicTexture != null ? true : false,
                                                                   metallicTexture != null ? metallicTexture.GetInstanceID() : -1,
                                                                   roughnessTexture != null ? true : false,
                                                                   roughnessTexture != null ? roughnessTexture.GetInstanceID() : -1,
                                                                   ambientOcclusionTexture != null ? true : false,
                                                                   ambientOcclusionTexture != null ? ambientOcclusionTexture.GetInstanceID() : -1);
        }

        public void Update()
        {
            if(_albedoTextureMonitor.CheckForUpdates() && albedoTexture != null)
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.AddTexture(albedoTexture.GetInstanceID(), albedoTexture.GetNativeTexturePtr());
            }

            if (_emissionTextureMonitor.CheckForUpdates() && emissionTexture != null)
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.AddTexture(emissionTexture.GetInstanceID(), emissionTexture.GetNativeTexturePtr());
            }

            if (_normalTextureMonitor.CheckForUpdates() && normalTexture != null)
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.AddTexture(normalTexture.GetInstanceID(), normalTexture.GetNativeTexturePtr());
            }

            if (_metallicTextureMonitor.CheckForUpdates() && metallicTexture != null)
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.AddTexture(metallicTexture.GetInstanceID(), metallicTexture.GetNativeTexturePtr());
            }

            if (_roughnessTextureMonitor.CheckForUpdates() && roughnessTexture != null)
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.AddTexture(roughnessTexture.GetInstanceID(), roughnessTexture.GetNativeTexturePtr());
            }
            
            if (_ambientOcclusionTextureMonitor.CheckForUpdates() && ambientOcclusionTexture != null)
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.AddTexture(ambientOcclusionTexture.GetInstanceID(), ambientOcclusionTexture.GetNativeTexturePtr());
            }

            if (_materialMonitor.CheckForUpdates())
            {
                PixelsForGlory.RayTracing.RayTracingPlugin.UpdateMaterial(InstanceId,
                                                                          albedo.r, albedo.g, albedo.b,
                                                                          emission.r, emission.g, emission.b,
                                                                          transmittance.r, transmittance.g, transmittance.b,
                                                                          metallic,
                                                                          roughness,
                                                                          indexOfRefraction,
                                                                          albedoTexture != null ? true : false,
                                                                          albedoTexture != null ? albedoTexture.GetInstanceID() : -1,
                                                                          emissionTexture != null ? true : false,
                                                                          emissionTexture != null ? emissionTexture.GetInstanceID() : -1,
                                                                          normalTexture != null ? true : false,
                                                                          normalTexture != null ? normalTexture.GetInstanceID() : -1,
                                                                          metallicTexture != null ? true : false,
                                                                          metallicTexture != null ? metallicTexture.GetInstanceID() : -1,
                                                                          roughnessTexture != null ? true : false,
                                                                          roughnessTexture != null ? roughnessTexture.GetInstanceID() : -1,
                                                                          ambientOcclusionTexture != null ? true : false,
                                                                          ambientOcclusionTexture != null ? ambientOcclusionTexture.GetInstanceID() : -1);
            }

            
        }
    }
}