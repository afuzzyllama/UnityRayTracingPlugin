#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "../Vulkan/ShaderConstants.h"


layout(set = DESCRIPTOR_SET_VERTEX_ATTRIBUTES, binding = DESCRIPTOR_BINDING_VERTEX_ATTRIBUTES, std430) readonly buffer AttribsBuffer {
    ShaderVertexAttribute VertexAttribs[];
} AttribsArray[];

layout(set = DESCRIPTOR_SET_FACE_DATA, binding = DESCRIPTOR_BINDING_FACE_DATA, std430) readonly buffer FacesBuffer {
    ShaderFace Faces[];
} FacesArray[];

layout(location = LOCATION_PRIMARY_RAY) rayPayloadInEXT ShaderRayPayload PrimaryRay;
                                        hitAttributeEXT vec2 HitAttribs;

void main() {
    
    // Return payload to gen shader
    PrimaryRay.albedo = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    PrimaryRay.distance = gl_HitTEXT;
}

