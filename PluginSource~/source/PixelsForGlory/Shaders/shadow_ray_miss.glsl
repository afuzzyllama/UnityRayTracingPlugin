#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require

#include "../Vulkan/ShaderConstants.h"

layout(location = LOCATION_SHADOW_RAY) rayPayloadInEXT ShaderShadowRayPayload ShadowRay;

void main() {
    // We didn't hit anything, so just return indicating that
    ShadowRay.distance = -1.0f;
}
