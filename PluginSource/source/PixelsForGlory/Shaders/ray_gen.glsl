#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require

#include "../Vulkan/ShaderConstants.h"

layout(set = DESCRIPTOR_SET_ACCELERATION_STRUCTURE, binding = DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE)        uniform accelerationStructureEXT Scene;

layout(set = DESCRIPTOR_SET_RENDER_TARGET,         binding = DESCRIPTOR_BINDING_RENDER_TARGET, rgba8)    uniform image2D RenderTarget;

layout(set = DESCRIPTOR_SET_SCENE_DATA,             binding = DESCRIPTOR_BINDING_SCENE_DATA, std140)            uniform SceneData {
    ShaderSceneParam SceneParams;
};

layout(set = DESCRIPTOR_SET_CAMERA_DATA,            binding = DESCRIPTOR_BINDING_CAMERA_DATA, std140)           uniform CameraData {
    ShaderCameraParam CameraParams;
};

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
    vec3 u = CameraParams.camSide.xyz;
    vec3 v = CameraParams.camUp.xyz;

    const float planeWidth = tan(CameraParams.camNearFarFov.z * 0.5f);

    u *= (planeWidth * aspect);
    v *= planeWidth;

    const vec3 rayDir = normalize(CameraParams.camDir.xyz + (u * screenUV.x) - (v * screenUV.y));
    return rayDir;
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
                    CameraParams.camNearFarFov.y, // camera.Far = tmax
                    LOCATION_PRIMARY_RAY);

        const float hitDistance = PrimaryRay.distance;
        rays[currentRayIndex].color = PrimaryRay.albedo.rgb;
        
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
    vec3 origin = CameraParams.camPos.xyz;
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
