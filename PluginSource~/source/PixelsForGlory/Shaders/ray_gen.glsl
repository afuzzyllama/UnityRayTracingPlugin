#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "../Vulkan/ShaderConstants.h"

layout(set = DESCRIPTOR_SET_ACCELERATION_STRUCTURE, binding = DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE)        uniform accelerationStructureEXT Scene;
layout(set = DESCRIPTOR_SET_RENDER_TARGET,          binding = DESCRIPTOR_BINDING_RENDER_TARGET,         rgba8)  uniform image2D RenderTarget;
layout(set = DESCRIPTOR_SET_SCENE_DATA,             binding = DESCRIPTOR_BINDING_SCENE_DATA,            std140) uniform SceneDataType { ShaderSceneData SceneData; };
layout(set = DESCRIPTOR_SET_CAMERA_DATA,            binding = DESCRIPTOR_BINDING_CAMERA_DATA,           std140) uniform CameraDataType { ShaderCameraData CameraData; };
layout(set = DESCRIPTOR_SET_LIGHTS_DATA,            binding = DESCRIPTOR_BINDING_LIGHTS_DATA,           std430) readonly buffer ShaderLightingBuffer { ShaderLightingData Light; } LightArray[];
layout(set = DESCRIPTOR_SET_MATERIALS_DATA,         binding = DESCRIPTOR_BINDING_MATERIALS_DATA,        std430) readonly buffer MaterialDataBuffer { ShaderMaterialData MaterialData; } MaterialDataArray[];

layout(location = LOCATION_PRIMARY_RAY) rayPayloadEXT ShaderRayPayload PrimaryRay;
layout(location = LOCATION_SHADOW_RAY)  rayPayloadEXT ShaderShadowRayPayload ShadowRay;

const uint rayFlags = gl_RayFlagsOpaqueEXT;
const uint shadowRayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT;
const uint cullMask = 0xFF;
const uint sbtRecordStride = 1;
const float tmin = 0.0f;
const float AIR_REFRACTICE_INDEX = 1.0003f;
const float PI = 3.1415926535897932384626433832795f;

const float nudgeFactor = 0.01f;


// Structure that represnts a cast ray
struct RayCastData {
    vec3 origin;
    vec3 direction;
    vec3 attenuation;
    uint recursionLevel;
    float threshold;
    float indexOfRefraction;
    bool inside;
    int parentIndex;
    vec3 globalReflectivity;
    vec3 globalTransmission;
    vec3 color;
};

// Array to store "recursive" ray calls
RayCastData rays[2 ^ RAYTRACE_MAX_RECURSION - 1];
int rayCount = 0;


// Calculate the ray direction for the hit
vec3 CalcRayDir(vec2 screenUV, float aspect) {
    vec3 u = CameraData.right.xyz;
    vec3 v = CameraData.up.xyz;

    const float planeWidth = tan(CameraData.fieldOfView * 0.5f);

    u *= (planeWidth * aspect);
    v *= planeWidth;

    const vec3 rayDir = normalize(CameraData.direction.xyz + (u * screenUV.x) - (v * screenUV.y));
    return rayDir;
}

// Figure out if a hit is in shadow
bool InShadow(in vec3 hitPosition, in vec3 hitNormal, in uint lightType, in vec3 lightPosition, in vec3 lightColor, in float distance, inout vec3 shadowColor)
{
    // Send out shadow feeler!
    vec3 shadowRayOrigin = hitPosition + hitNormal * nudgeFactor; // Nudge shadow ray slightly
    vec3 shadowRayDirection = lightPosition - hitPosition;
    shadowRayDirection = normalize(shadowRayDirection);

    shadowColor = vec3(0.0f);

    bool hit = false;

    traceRayEXT(Scene,
        shadowRayFlags,
        cullMask,
        SHADOW_HIT_SHADERS_INDEX,
        sbtRecordStride,
        SHADOW_MISS_SHADERS_INDEX,
        shadowRayOrigin,
        0.0f,
        shadowRayDirection, 
        (lightType == SHADER_LIGHTTYPE_POINT ? distance : CameraData.farClipPlane), // tmax,
        LOCATION_SHADOW_RAY);

    bool firstHit = true;
    while(ShadowRay.distance > 0.0f)
    {
        // We hit something
        hit = true;
        
        ShaderMaterialData hitMaterial = MaterialDataArray[ShadowRay.materialIndex].MaterialData;

        // Figure out if light can pass through this material or not
        if(hitMaterial.transmittance.r == 0.0f && hitMaterial.transmittance.g == 0.0f && hitMaterial.transmittance.b == 0.0f)
        {
            // light cannot pass through material, make the shadow black
            shadowColor = vec3(0.0f);
            break;
        }
        
      if(firstHit == true) 
      {
          // If this is the first hit, set the shadow color to the calculated light color
          shadowColor = lightColor;
          firstHit = false;
      }
  
      // Mix the current color and the shadow hit color by 1.0 - transmittance to get the shadow's influence on the current color
      // Then attenuate by the transmittance
      shadowColor = vec3(mix(shadowColor.r, ShadowRay.albedo.r, 1.0f - hitMaterial.transmittance.r), mix(shadowColor.g, ShadowRay.albedo.g, 1.0f - hitMaterial.transmittance.g), mix(shadowColor.b, ShadowRay.albedo.b, 1.0f - hitMaterial.transmittance.b)) * hitMaterial.transmittance.rgb;
 
      // Send another ray out to see if we hit anything else
      shadowRayOrigin = (shadowRayOrigin + shadowRayDirection * ShadowRay.distance) + hitNormal * nudgeFactor; // Nudge shadow ray slightly
      distance = length(lightPosition - shadowRayOrigin);  // Recalculate the distance for tmax from the current hit object
      traceRayEXT(Scene,
          shadowRayFlags,
          cullMask,
          SHADOW_HIT_SHADERS_INDEX,
          sbtRecordStride,
          SHADOW_MISS_SHADERS_INDEX,
          shadowRayOrigin,
          0.0f,
          shadowRayDirection,
          (lightType == SHADER_LIGHTTYPE_POINT ? distance : CameraData.farClipPlane), // tmax,
          LOCATION_SHADOW_RAY);
    }
  
    return hit;
}


// Normal Distribution Function, which accounts for the fraction of facets which reflect light at the viewer. Defines the shape of the specular highlight.
// Trowbridge-Reitz implementation
float DistributionGGX(vec3 N, vec3 H, float alpha)
{
    float alpha2 = alpha * alpha;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;
    
    float num   = alpha2;
    float denom = NdotH2 * (alpha2 - 1.0f) + 1.0f;
    denom = PI * denom * denom;
    return num / denom;
}

// Geometric Attenuation Function, which accounts for the shadowing and masking of facets by one another.
// Schlick-GGX
float GeometricG(vec3 X, vec3 N, float alpha)
{
    // Direct lighting coefficient
    float k = (alpha + 1.0f) * (alpha + 1.0f) / 8.0f;
    float XdotN = max(dot(X, N), 0.0f);

    return XdotN / (XdotN * (1.0f - k) + k);
}

float GeometricGGX(vec3 V, vec3 L, vec3 N, float alpha)
{
    // Get the geometric contribution from the view and the light
    return GeometricG(V, N, alpha) * GeometricG(L, N, alpha);
}

// Approximation for the fresnel effect
vec3 FresnelSchlick(vec3 V, vec3 H, vec3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - max(dot(V, H), 0.0f), 5.0f);
}

// Cook-Torrance Physically Based Rendering lighting
vec3 CookTorrancePBR(in vec3 hitPosition, in vec3 hitNormal, in vec4 emission, in vec4 albedo, in float metallic, in float roughness, in float ambientOcclusion, in float indexOfRefraction) {

    vec3 N = normalize(hitNormal);
    vec3 V = normalize(CameraData.position.xyz - hitPosition);
    
    // Compute the "base reflectivity" for the surface, which will be used for the Fresnel calculation later
    // "The idea is that if the material is dielectric we use the IOR, otherwise we use the albedo color to "tint" the reflection.
    // Ignore the IOR value if metallic is set to 1, and we use the albedo colour instead.
    vec3 F0 = vec3((indexOfRefraction - 1.0f) / (indexOfRefraction + 1.0f));
    F0 = F0 * F0;
    F0 = mix(F0, albedo.rgb, metallic);

    // Adapted from:
    // Graphics Programming Compendium
    // https://graphicscompendium.com/gamedev/15-pbr
    // https://learnopengl.com/PBR/Lighting
    // http://www.codinglabs.net/article_physically_based_rendering_cook_torrance.aspx
    // r = ka + sum(lc * (n dot l) * (d * rd + s * rs))
    vec3 Lo = vec3(0.0f);
    for(uint i = 0; i < SceneData.lightCount; ++i) 
    {
        if(!LightArray[i].Light.enabled)
        {
            continue;
        }

        vec3 L = normalize(LightArray[i].Light.position - hitPosition);
        vec3 H = normalize(V + L);
        float distance = length(LightArray[i].Light.position - hitPosition);

        vec3 lc = LightArray[i].Light.color;
        vec3 rd = albedo.rgb;

        float D = DistributionGGX(N, H, roughness);
        float G = GeometricGGX(V, L, N, roughness);
        vec3 F = FresnelSchlick(V, H, F0);

        vec3 s = vec3(F);
        vec3 d = vec3(1.0f) - s;
        d *= 1.0 - metallic;	

        float denom = 4.0f * max(dot(N, L), 0.0f) * max(dot(N, V), 0.0f);
        vec3 rs = (D * G * F) / max(denom, 0.001f);
        vec3 lightContribution = lc * max(dot(N, L), 0.0f) * (d * rd + s * rs);


        // Send out shadow feeler!
        // Tracing shadow ray only if the light is visible from the surface
        if(dot(N, L) > 0.0f) {
            vec3 shadowColor;
            if (InShadow(hitPosition, hitNormal, LightArray[i].Light.type, LightArray[i].Light.position, lightContribution, distance, shadowColor))
            {
                // If we hit something, modular the final calculated color
                Lo += shadowColor;
            }
            else 
            {
                Lo += lightContribution;
            }
        }
        else 
        {
            Lo += lightContribution;
        }
    }   
  
    // Calcuate the ambient contribution
    vec3 ambient = emission.rgb + (SceneData.ambient.rgb * albedo.rgb * ambientOcclusion);
    
    // Add the light contribution to ambient.  If negative Lo, just consider it no contribution
    vec3 color = ambient.rgb + Lo;

    

    return color;
}

// Is a ray below the depth threshold?
bool BelowThreshold(in RayCastData ray) {
    // TODO: pass as parameter later
    return ray.attenuation.r < 0.025f && 
           ray.attenuation.g < 0.025f && 
           ray.attenuation.b < 0.025f;


    // (ray.attenuation.r < SceneParams.depthThreshold  &&
//      ray.attenuation.g < SceneParams.depthThreshold  &&
//      ray.attenuation.b < SceneParams.depthThreshold);
}

// Get a reflected ray based on input
RayCastData GetReflectedRay(in RayCastData parentRay, in vec3 intPt, in vec3 normal) 
{
    RayCastData reflectedRay;
    reflectedRay.inside = false;                                    // Reflected rays should never be inside another object
    reflectedRay.recursionLevel = parentRay.recursionLevel - 1;     // Reduce the recursion level
    reflectedRay.threshold = parentRay.threshold;                   // Use parent threshold as this ray's threshold
    reflectedRay.origin = intPt + normal * nudgeFactor;                  // nudge ray before reflecting
    reflectedRay.direction = reflect(parentRay.direction, normal);  // Reflect the ray using glsl method

    return reflectedRay;
}

// Get a refracted ray based on input
RayCastData GetRefractedRay(in RayCastData parentRay, in vec3 intPt, in vec3 n, in ShaderMaterialData m, inout bool totalInternalReflection)
{
    RayCastData refractedRay;
    refractedRay.recursionLevel = parentRay.recursionLevel - 1;     // Reduce the recursion level
    refractedRay.threshold = parentRay.threshold;                   // Use parent threshold as this ray's threshold

    if(parentRay.inside) {
        // Leaving object
        refractedRay.indexOfRefraction = AIR_REFRACTICE_INDEX;
        refractedRay.inside = false;

        float u = m.indexOfRefraction / parentRay.indexOfRefraction;
        vec3 i = normalize(intPt - parentRay.origin);
        float iDotN = max(dot(i, n), 0.0f);
        float tirValue = 1.0f - (u * u) * (1.0f - (iDotN * iDotN));
        
        if(tirValue < 0.0f) {
            totalInternalReflection = true;
            // If TIR, don't bother calculating new ray
        }
        else {
            totalInternalReflection = false;
            refractedRay.direction = refract(parentRay.direction, n, u);    // Refract ray using glsl method
        }
    }
    else {
        // Entering object. Total Internal Reflection is not possible
        totalInternalReflection = false;

        refractedRay.indexOfRefraction = m.indexOfRefraction;
        refractedRay.inside = true;

        float u = parentRay.indexOfRefraction/m.indexOfRefraction;
        refractedRay.direction = refract(parentRay.direction, n, u);        // Refract ray using glsl method
    }

    refractedRay.origin = intPt + refractedRay.direction * nudgeFactor; // nudge ray
    return refractedRay;
}

// Trace a ray
vec3 TraceRay() {
    int currentRayIndex = 0;

    // Since we cannot use recursion, process all rays added to "rays" until there are no rays left to process
    while(currentRayIndex < rayCount)
    {
        const vec3 origin = rays[currentRayIndex].origin;
        const vec3 direction = rays[currentRayIndex].direction;

        // Real time ray tracing!
        traceRayEXT(Scene,
                    rayFlags,
                    cullMask,
                    PRIMARY_HIT_SHADERS_INDEX,
                    sbtRecordStride,
                    PRIMARY_MISS_SHADERS_INDEX,
                    origin,
                    tmin,
                    direction,
                    CameraData.farClipPlane, // camera.Far = tmax
                    LOCATION_PRIMARY_RAY);

        const float hitDistance = PrimaryRay.distance;
        rays[currentRayIndex].color = PrimaryRay.albedo.rgb;
        if (hitDistance < 0.0f) {
            // Ray missed, nothing else to process
            rays[currentRayIndex].color = SceneData.ambient.rgb;
            currentRayIndex++;
            continue;
        } 

        // If we got here, we hit something!
        // Gather information from hit
        const vec3 hitPosition = origin + direction * hitDistance;
        const vec3 hitNormal = PrimaryRay.normal.xyz;
        const ShaderMaterialData material = MaterialDataArray[PrimaryRay.materialIndex].MaterialData;
               
        // Get ray color from lighting
        rays[currentRayIndex].color = CookTorrancePBR(hitPosition, hitNormal, PrimaryRay.emission, PrimaryRay.albedo, PrimaryRay.metallic, PrimaryRay.roughness, PrimaryRay.ambientOcclusion, material.indexOfRefraction);
        
        int parentIndex = currentRayIndex;
        if(rays[currentRayIndex].recursionLevel == 0 || BelowThreshold(rays[currentRayIndex]))
        {
            // Return if max depth is reached or attenuation is below threshold (do not spawn additional rays)
             break;
        }

        // Reflected ray (do not spawn if inside an object). Only reflect if metallic.
        if (!rays[currentRayIndex].inside && material.metallic > 0.0f)
        {
            // Get the reflected ray, attenuate it, and trace it to get a color
            RayCastData reflectedRay = GetReflectedRay(rays[currentRayIndex], hitPosition, hitNormal);

            // Attenuate the ray and trace the transmitted ray
            reflectedRay.parentIndex = parentIndex;
            reflectedRay.attenuation = rays[currentRayIndex].attenuation * material.metallic;
            reflectedRay.globalTransmission = vec3(0.0f);
            reflectedRay.globalReflectivity = vec3(material.metallic);

            // Add ray for processing
            rays[rayCount] = reflectedRay;
            ++rayCount;
        }

        // Transmitted ray
        if (material.transmittance.r > 0.0f || material.transmittance.g > 0.0f || material.transmittance.b > 0.0f)
        {
            vec3 calculatedNormal = hitNormal;
             // Reverse the normal direction if the ray is inside
             if (rays[currentRayIndex].inside) {
                 calculatedNormal *= -1.0f;
             }

             bool tir = false;
             RayCastData transmittedRay = GetRefractedRay(rays[currentRayIndex], hitPosition, calculatedNormal, material, tir);

             // Do not trace if total intenal reflection
             if (!tir)
             {
                // Attenuate the ray and trace the transmitted ray
                transmittedRay.parentIndex = parentIndex;
                transmittedRay.attenuation = rays[currentRayIndex].attenuation * material.transmittance.rgb;
                transmittedRay.globalTransmission = material.transmittance.rgb;
                transmittedRay.globalReflectivity = vec3(0.0f);

                // Add ray for processing
                rays[rayCount] = transmittedRay;
                rayCount++;
             }
        }
        
        // We're done here, process the next ray
        currentRayIndex++;
    }   
    
    // Calcuate final color for ray based on all cast rays
    vec3 finalColor = vec3(0.0f);
    for(int i = rayCount - 1; i >= 0; --i) {
        if(i == 0) {
            // We are at the root node, get the color and we're done!
            finalColor = rays[0].color;
            break;
        }

        // Add transmitted ray color
        rays[rays[i].parentIndex].color += rays[i].color * rays[i].globalTransmission;

        // Add reflected ray color
        rays[rays[i].parentIndex].color += rays[i].color * rays[i].globalReflectivity;
    }

    // Clamp color
    clamp(finalColor, vec3(0.0f), vec3(1.0f));

    return finalColor;
}

void main() {

    // Get current pixel information    
    const vec2 curPixel = vec2(gl_LaunchIDEXT.xy);
    const vec2 bottomRight = vec2(gl_LaunchSizeEXT.xy - 1);
    const vec2 uv = (curPixel / bottomRight) * 2.0f - 1.0f;
    const float aspect = float(gl_LaunchSizeEXT.x) / float(gl_LaunchSizeEXT.y);

    // Kick off root ray!
    vec3 origin = CameraData.position.xyz;
    vec3 direction = CalcRayDir(uv, aspect);

    RayCastData originRay;
    originRay.origin = origin;
    originRay.direction = direction;
    originRay.attenuation = vec3(1.0f);
    originRay.inside = false;
    originRay.recursionLevel = RAYTRACE_MAX_RECURSION;
    originRay.parentIndex = -1;
    originRay.indexOfRefraction = AIR_REFRACTICE_INDEX;
    originRay.color = vec3(0.0f);

    rays[0] = originRay;
    rayCount = 1;
 
    vec3 finalColor = TraceRay();

    // Return result to image      
    imageStore(RenderTarget, ivec2(gl_LaunchIDEXT.xy), vec4(finalColor, 1.0f));

}
