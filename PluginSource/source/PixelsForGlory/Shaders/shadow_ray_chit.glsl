#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "../Vulkan/ShaderConstants.h"

// layout(set = SWS_MATIDS_SET, binding = 0, std430) readonly buffer MatIDsBuffer {
//     uint MatIDs[];
// } MatIDsArray[];

layout(set = DESCRIPTOR_SET_VERTEX_ATTRIBUTES, binding = DESCRIPTOR_BINDING_VERTEX_ATTRIBUTES, std430) readonly buffer AttribsBuffer {
    ShaderVertexAttribute VertexAttribs[];
} AttribsArray[];

layout(set = DESCRIPTOR_SET_FACE_DATA, binding = DESCRIPTOR_BINDING_FACE_DATA, std430) readonly buffer FacesBuffer {
    ShaderFace Faces[];
} FacesArray[];

// layout(set = SWS_MATERIALDATA_SET,  binding = SWS_MATERIALDATA_BINDING, std140) uniform MaterialData {
//     MaterialParam MaterialParams[MAX_MATERIALS];
// };

// layout(set = SWS_TEXTURES_SET, binding = 0) uniform sampler2D TexturesArray[];

layout(location = LOCATION_SHADOW_RAY)  rayPayloadInEXT ShaderShadowRayPayload ShadowRay;
                                        hitAttributeEXT vec2 HitAttribs;

void main() {
     ShadowRay.distance = gl_HitTEXT;
}
