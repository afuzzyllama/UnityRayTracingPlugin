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


// layout(set = SWS_MATERIALDATA_SET,  binding = SWS_MATERIALDATA_BINDING, std140) uniform MaterialData {
//     MaterialParam MaterialParams[MAX_MATERIALS];
// };

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
    vec3 u = CameraData.camSide.xyz;
    vec3 v = CameraData.camUp.xyz;

    const float planeWidth = tan(CameraData.camNearFarFov.z * 0.5f);

    u *= (planeWidth * aspect);
    v *= planeWidth;

    const vec3 rayDir = normalize(CameraData.camDir.xyz + (u * screenUV.x) - (v * screenUV.y));
    return rayDir;
}

// // Figure out if a hit is in shadow
// bool InShadow(in vec3 hitPos, in vec3 hitNormal, in uint lightType, in vec3 lightPos, in vec3 lightColor, in float distance, inout vec3 shadowColor)
// {
//     // Send out shadow feeler!
//     vec3 shadowRayOrigin = hitPos + hitNormal * nudgeFactor; // Nudge shadow ray slightly
//     vec3 shadowRayDirection = lightPos - hitPos;
//     shadowRayDirection = normalize(shadowRayDirection);

//     shadowColor = vec3(0.0f);

//     bool hit = false;

//     traceRayEXT(Scene,
//         shadowRayFlags,
//         cullMask,
//         SWS_SHADOW_HIT_SHADERS_IDX,
//         sbtRecordStride,
//         SWS_SHADOW_MISS_SHADERS_IDX,
//         shadowRayOrigin,
//         0.0f,
//         shadowRayDirection, 
//         (lightType == LIGHT_TYPE_POINT ? distance : CameraParams.camNearFarFov.y), // tmax,
//         SWS_LOC_SHADOW_RAY);

//     bool firstHit = true;
//     while(ShadowRay.distance > 0.0f)
//     {
//         // We hit something
//         hit = true;
        
//         MaterialParam hitMaterial = MaterialParams[ShadowRay.matId];

//         // Figure out if light can pass through this material or not
//         if(hitMaterial.transmittance.r == 0.0f && hitMaterial.transmittance.g == 0.0f && hitMaterial.transmittance.b == 0.0f)
//         {
//             // light cannot pass through material, make the shadow black
//             shadowColor = vec3(0.0f);
//             break;
//         }
        
//         if(firstHit == true) 
//         {
//             // If this is the first hit, set the shadow color to the calculated light color
//             shadowColor = lightColor;
//             firstHit = false;
//         }
        
//         // Mix the current color and the shadow hit color by 1.0 - transmittance to get the shadow's influence on the current color
//         // Then attenuate by the transmittance
//         shadowColor = vec3(mix(shadowColor.r, ShadowRay.albedo.r, 1.0f - hitMaterial.transmittance.r), mix(shadowColor.g, ShadowRay.albedo.g, 1.0f - hitMaterial.transmittance.g), mix(shadowColor.b, ShadowRay.albedo.b, 1.0f - hitMaterial.transmittance.b)) * hitMaterial.transmittance.rgb;
       
//         // Send another ray out to see if we hit anything else
//         shadowRayOrigin = (shadowRayOrigin + shadowRayDirection * ShadowRay.distance) + hitNormal * nudgeFactor; // Nudge shadow ray slightly
//         distance = length(lightPos - shadowRayOrigin);  // Recalculate the distance for tmax from the current hit object
//         traceRayEXT(Scene,
//             shadowRayFlags,
//             cullMask,
//             SWS_SHADOW_HIT_SHADERS_IDX,
//             sbtRecordStride,
//             SWS_SHADOW_MISS_SHADERS_IDX,
//             shadowRayOrigin,
//             0.0f,
//             shadowRayDirection,
//             (lightType == LIGHT_TYPE_POINT ? distance : CameraParams.camNearFarFov.y), // tmax,
//             SWS_LOC_SHADOW_RAY);
//     }
        
//     return hit;
// }

// // Calculate the specular contribution of the PBR 
// // Based on "microfacets":
// // If a surface is smoother, light will not scatter is much
// // If a surface is rougher, light will scatter and have no specular contribution
// // Based on Trowbridge-Reitz GGX
// float DistributionGGX(vec3 N, vec3 H, float a)
// {
//     float a2     = a*a;
//     float NdotH  = max(dot(N, H), 0.0);
//     float NdotH2 = NdotH*NdotH;
	
//     float nom    = a2;
//     float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
//     denom        = PI * denom * denom;
	
//     return nom / denom;
// }

// // The distribution used by Smith's method to calculate diffuse shadowing
// // Based on Schlick-GGX
// float GeometrySchlickGGX(float NdotV, float k)
// {
//     float nom   = NdotV;
//     float denom = NdotV * (1.0 - k) + k;
	
//     return nom / denom;
// }

// // Calculates the diffuse shadowing contribution of the PBR
// // Based on "microfacets":
// // If a surface is smooth, there will be little self shadowing and most of the surface will be the albedo color
// // If a surface is rough, there will be more self shadowing over the surface darkening the albedo color
// // Based on Smith's method
// float GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
// {
//     float NdotV = max(dot(N, V), 0.0);
//     float NdotL = max(dot(N, L), 0.0);
//     float ggx1 = GeometrySchlickGGX(NdotV, k);
//     float ggx2 = GeometrySchlickGGX(NdotL, k);
	
//     return ggx1 * ggx2;
// }

// // Calcuate the Fresnel contribution.  Used with metallic value to define how much light is absorbed by the surface
// // before reflection.
// vec3 FresnelSchlick(float cosTheta, vec3 F0)
// {
//     return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
// }

vec3 SimpleLighting(vec3 hitPosition, vec3 hitNormal)
{
    vec3 finalColor = vec3(0.0f);

    for(int i = 0; i < 2; ++i)
    {
        ShaderLightingData light = LightArray[i].Light;
     
        if(!light.enabled)
        {
            continue;
        }

        vec3 norm = normalize(hitNormal);
        vec3 lightDir = normalize(light.position - hitPosition);
        float diffuse = max(dot(norm, lightDir), 0.0f);
        
	    finalColor += diffuse * light.color;
    }

    return finalColor;
}

// // Cook-Torrance Physically Based Rendering lighting
// vec3 CookTorrancePBR(in vec3 hitPos, in vec3 n, in vec3 emission, in vec3 albedo, in float metallic, in vec3 roughness, in vec3 ao) {

//     // Adapted from:
//     // https://learnopengl.com/PBR/Lighting
//     vec3 N = normalize(n);
//     vec3 V = normalize(CameraParams.camPos.xyz - hitPos);
    
//     // Compute the "base reflectivity" for the surface, which will be used for the
//     // Fresnel calculation later
//     vec3 F0 = vec3(0.04); 
//     F0 = mix(F0, albedo, metallic);
	           
//     // ReflectanceEequation
//     vec3 Lo = vec3(0.0f);

//     // Iterate through each light to get its contribution to the scene
//     for(int i = 0; i < SceneParams.lightCount; ++i) 
//     {
//         uint lightType = LightingParams[i].type;
//         vec3 lightColor = LightingParams[i].color.rgb;
//         vec3 lightPosition = LightingParams[i].position.xyz;
        
//         vec3 L = normalize(lightPosition - hitPos);
//         vec3 H = normalize(V + L);
//         float distance    = length(lightPosition - hitPos);
       
//         // Calculate per-light radiance
//         float attenuation = 1.0f;
//         if(lightType == LIGHT_TYPE_POINT) {
//             // Adapted from:
//             // https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/shading-spherical-light
            
//             // If we are working with a point light, attenuate based on intensity
//             attenuation = LightingParams[i].intensity / (4 * PI * distance * distance);
//         }

//         vec3 radiance     = lightColor * attenuation;        
        
//         // Cook-Torrance BRDF
//         float NDF = DistributionGGX(N, H, roughness.r);        
//         float G   = GeometrySmith(N, V, L, roughness.r);      
//         vec3 F    = FresnelSchlick(max(dot(H, V), 0.0f), F0);       
        
//         vec3 kS = F;
//         vec3 kD = vec3(1.0f) - kS;
//         kD *= (1.0 - metallic);	  
        
//         vec3  numerator    = NDF * G * F;
//         float denominator = 4.0 * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f);
//         vec3  specular     = numerator / max(denominator, 0.001f);  
        
//         // Add to outgoing radiance Lo
//         float NdotL = max(dot(N, L), 0.0f);  
//         vec3 colorContribution = (kD * albedo / PI + specular) * radiance * NdotL;            

//         // Send out shadow feeler!
//         // Tracing shadow ray only if the light is visible from the surface
//         if(dot(N, L) > 0.0f) {
//             vec3 shadowColor;
//             if (InShadow(hitPos, n, lightType, lightPosition, colorContribution, distance, shadowColor))
//             {
//                 // If we hit something, modular the final calculated color
//                 Lo += shadowColor;
//             }
//             else 
//             {
//                 Lo += colorContribution;
//             }
//         }
//         else 
//         {
//             Lo += colorContribution;
//         }
//     }   
  
//     // Calcuate the ambient contribution
//     vec3 ambient = emission + (SceneParams.ambient.rgb * albedo * ao);
    
//     // Add the light contribution to ambient.  If negative Lo, just consider it no contribution
//     vec3 color = ambient + max(Lo, vec3(0.0f));
	
//     // Gamma correct the color to be more visually appealing.
//     color = color / (color + vec3(1.0));
//     color = pow(color, vec3(1.0/1.2));  
   
//     return color;
// }

// // Is a ray below the depth threshold?
// bool BelowThreshold(in RayCastData ray) {
//     return (ray.attenuation.r < SceneParams.depthThreshold  &&
//               ray.attenuation.g < SceneParams.depthThreshold  &&
//               ray.attenuation.b < SceneParams.depthThreshold);
// }

// // Get a reflected ray based on input
// RayCastData GetReflectedRay(in RayCastData parentRay, in vec3 intPt, in vec3 normal) 
// {
//     RayCastData reflectedRay;
//     reflectedRay.inside = false;                                    // Reflected rays should never be inside another object
//     reflectedRay.recursionLevel = parentRay.recursionLevel - 1;     // Reduce the recursion level
//     reflectedRay.threshold = parentRay.threshold;                   // Use parent threshold as this ray's threshold
//     reflectedRay.origin = intPt + normal * nudgeFactor;                  // nudge ray before reflecting
//     reflectedRay.direction = reflect(parentRay.direction, normal);  // Reflect the ray using glsl method

//     return reflectedRay;
// }

// // Get a refracted ray based on input
// RayCastData GetRefractedRay(in RayCastData parentRay, in vec3 intPt, in vec3 n, in MaterialParam m, inout bool totalInternalReflection)
// {
//     RayCastData refractedRay;
//     refractedRay.recursionLevel = parentRay.recursionLevel - 1;     // Reduce the recursion level
//     refractedRay.threshold = parentRay.threshold;                   // Use parent threshold as this ray's threshold

//     if(parentRay.inside) {
//         // Leaving object
//         refractedRay.indexOfRefraction = AIR_REFRACTICE_INDEX;
//         refractedRay.inside = false;

//         float u = m.ior / parentRay.indexOfRefraction;
//         vec3 i = normalize(intPt - parentRay.origin);
//         float iDotN = dot(i, n);
//         float tirValue = 1.0f - (u * u) * (1.0f - (iDotN * iDotN));
        
//         if(tirValue < 0.0f) {
//             totalInternalReflection = true;
//             // If TIR, don't bother calculating new ray
//         }
//         else {
//             totalInternalReflection = false;
//             refractedRay.direction = refract(parentRay.direction, n, u);    // Refract ray using glsl method
//         }
//     }
//     else {
//         // Entering object. Total Internal Reflection is not possible
//         totalInternalReflection = false;

//         refractedRay.indexOfRefraction = m.ior;
//         refractedRay.inside = true;

//         float u = parentRay.indexOfRefraction/m.ior;
//         refractedRay.direction = refract(parentRay.direction, n, u);        // Refract ray using glsl method
//     }

//     refractedRay.origin = intPt + refractedRay.direction * nudgeFactor; // nudge ray
//     return refractedRay;
// }

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
                    CameraData.camNearFarFov.y, // camera.Far = tmax
                    LOCATION_PRIMARY_RAY);

        const float hitDistance = PrimaryRay.distance;
        rays[currentRayIndex].color = PrimaryRay.albedo.rgb;
        if (hitDistance < 0.0f) {
            // Ray missed, nothing else to process
            rays[currentRayIndex].color = PrimaryRay.albedo.rgb;
            currentRayIndex++;
            continue;
        } 

        // If we got here, we hit something!
        // Gather information from hit
        // const uint matId = int(PrimaryRay.matId);
        // const MaterialParam material = MaterialParams[matId];

        const vec3 hitPos = origin + direction * hitDistance;
        const vec3 hitNormal = PrimaryRay.normal.xyz;
               
        // If hitDistance >= 0.0f, the hitColor is the result of the texture sample
        // const bool  textured = true;
        // const vec3  textureColor = PrimaryRay.albedo.rgb; 
        // const vec3  roughness = PrimaryRay.roughness.rgb;
        // const float metallic = PrimaryRay.metallic;
        // const vec3  ambientOcclusion = PrimaryRay.ao.rgb;
        
        // Get ray color from lighting
        rays[currentRayIndex].color = PrimaryRay.albedo.rgb * SimpleLighting(hitPos, hitNormal); //CookTorrancePBR(hitPos, hitNormal, material.emission.rgb, textureColor, metallic, roughness, ambientOcclusion);
        
        // int parentIndex = currentRayIndex;
        // if(rays[currentRayIndex].recursionLevel == 0 || BelowThreshold(rays[currentRayIndex]))
        // {
        //     // Return if max depth is reached or attenuation is below threshold (do not spawn additional rays)
        //      break;
        // }

        // // Reflected ray (do not spawn if inside an object). Only reflect if metallic.
        // if (!rays[currentRayIndex].inside && material.metallic > 0.0f)
        // {
        //     // Get the reflected ray, attenuate it, and trace it to get a color
        //     RayCastData reflectedRay = GetReflectedRay(rays[currentRayIndex], hitPos, hitNormal);

        //     // Attenuate the ray and trace the transmitted ray
        //     reflectedRay.parentIndex = parentIndex;
        //     reflectedRay.attenuation = rays[currentRayIndex].attenuation * material.metallic;
        //     reflectedRay.globalTransmission = vec3(0.0f);
        //     reflectedRay.globalReflectivity = vec3(material.metallic);

        //     // Add ray for processing
        //     rays[rayCount] = reflectedRay;
        //     ++rayCount;
        // }

        // // Transmitted ray
        // if (material.transmittance.r > 0.0f || material.transmittance.g > 0.0f || material.transmittance.b > 0.0f)
        // {
        //     vec3 normal = hitNormal;
        //      // Reverse the normal direction if the ray is inside
        //      if (rays[currentRayIndex].inside) {
        //          normal *= -1.0f;
        //      }

        //      bool tir = false;
        //      RayCastData transmittedRay = GetRefractedRay(rays[currentRayIndex], hitPos, normal, material, tir);

        //      // Do not trace if total intenal reflection
        //      if (!tir)
        //      {
        //         // Attenuate the ray and trace the transmitted ray
        //         transmittedRay.parentIndex = parentIndex;
        //         transmittedRay.attenuation = rays[currentRayIndex].attenuation * material.transmittance.rgb;
        //         transmittedRay.globalTransmission = material.transmittance.rgb;
        //         transmittedRay.globalReflectivity = vec3(0.0f);

        //         // Add ray for processing
        //         rays[rayCount] = transmittedRay;
        //         rayCount++;
        //      }
        // }
        
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
        // rays[rays[i].parentIndex].color += rays[i].color * rays[i].globalTransmission;

        // Add reflected ray color
        //rays[rays[i].parentIndex].color += rays[i].color * rays[i].globalReflectivity;
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
    vec3 origin = CameraData.camPos.xyz;
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
