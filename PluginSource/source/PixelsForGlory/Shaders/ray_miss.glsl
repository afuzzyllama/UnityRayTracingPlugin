#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require

#include "../Vulkan/ShaderConstants.h"

layout(location = LOCATION_PRIMARY_RAY) rayPayloadInEXT ShaderRayPayload PrimaryRay;

void main() {
    // If we miss, return black color
    // const vec3 backgroundColor = vec3(0.0f, 0.0f, 0.0f);
    PrimaryRay.albedo = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    // PrimaryRay.normal = vec4(0.0f);
    // PrimaryRay.ao = vec4(0.0f);
    PrimaryRay.distance = -1.0f;
    // PrimaryRay.matId = 0;
}
