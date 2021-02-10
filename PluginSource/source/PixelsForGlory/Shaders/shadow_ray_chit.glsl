#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "../Vulkan/ShaderConstants.h"

layout(set = DESCRIPTOR_SET_FACE_DATA,         binding = DESCRIPTOR_BINDING_FACE_DATA,         std430) readonly buffer FacesBuffer { ShaderFaceData Faces[]; } FacesArray[];
layout(set = DESCRIPTOR_SET_VERTEX_ATTRIBUTES, binding = DESCRIPTOR_BINDING_VERTEX_ATTRIBUTES, std430) readonly buffer VertexAttributesBuffer { ShaderVertexAttributeData VertexAttributes[]; } VertexAttributesArray[];
layout(set = DESCRIPTOR_SET_INSTANCE_DATA,     binding = DESCRIPTOR_BINDING_INSTANCE_DATA,     std430) readonly buffer InstanceDataBuffer { ShaderInstanceData InstanceData; } InstanceDataArray[];
layout(set = DESCRIPTOR_SET_MATERIALS_DATA,    binding = DESCRIPTOR_BINDING_MATERIALS_DATA,    std430) readonly buffer MaterialDataBuffer { ShaderMaterialData MaterialData; } MaterialDataArray[];

layout(location = LOCATION_SHADOW_RAY)  rayPayloadInEXT ShaderShadowRayPayload ShadowRay;
                                        hitAttributeEXT vec2 HitAttribs;

void main() {
    // The hitAttributeEXT (i.e. the barycentric coordinates) as provided by the built-in triangle-ray intersection test. 
    const vec3 barycentrics = vec3(1.0f - HitAttribs.x - HitAttribs.y, HitAttribs.x, HitAttribs.y);

    // Get data  associated with this hit
    ShaderFaceData face = FacesArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].Faces[gl_PrimitiveID];

    ShaderVertexAttributeData v0 = VertexAttributesArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].VertexAttributes[face.index0];
    ShaderVertexAttributeData v1 = VertexAttributesArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].VertexAttributes[face.index1];
    ShaderVertexAttributeData v2 = VertexAttributesArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].VertexAttributes[face.index2];

    ShaderInstanceData instance = InstanceDataArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].InstanceData;
    ShaderMaterialData material = MaterialDataArray[instance.materialIndex].MaterialData;

    // Using the barycentric coordinates, find the uv for this hit
    const vec2 uv = BaryLerp(v0.uv.xy, v1.uv.xy, v2.uv.xy, barycentrics);

    // // Return color information for hit for shadowed objects that allow light to pass through
    vec4 albedo = material.albedo;
    // if(mat.albedoIndex != -1)
    // {
    //     texel *= textureLod(TexturesArray[nonuniformEXT(mat.albedoIndex)], uv, 0.0f).rgb;
    // }

    ShadowRay.materialIndex = instance.materialIndex;
    ShadowRay.distance = gl_HitTEXT;
    ShadowRay.albedo = albedo;
}
