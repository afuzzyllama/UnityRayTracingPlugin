using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[CreateAssetMenu(menuName = "Ray Tracing/Material")]
public class RayTracerMaterial : ScriptableObject
{
    public Texture2D albedoTexture;
    public Color     albedo;

    public Texture2D emissionTexture;
    public Color emission; 

    public Texture2D normalTexture;

    public Texture2D metallicTexture;
    [Range(0.0f, 1.0f)] public float metallic;

    public Texture2D roughnessTexture;
    [Range(0.0f, 1.0f)] public float roughness;

    public Texture2D ambientOcclusionTexture;

    [Range(1.0f, float.MaxValue)] public float indexOfRefraction;

    [ReadOnly] public int InstanceId;
}
