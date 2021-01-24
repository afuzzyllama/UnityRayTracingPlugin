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
    // // The hitAttributeEXT (i.e. the barycentric coordinates) as provided by the built-in triangle-ray intersection test. 
    // const vec3 barycentrics = vec3(1.0f - HitAttribs.x - HitAttribs.y, HitAttribs.x, HitAttribs.y);

    // // Get the material associated with this hit
    // uint matId = MatIDsArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].MatIDs[gl_PrimitiveID];
    // const MaterialParam mat = MaterialParams[matId];

    // // Get the face associated with this hit
    // const uvec4 face = FacesArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].Faces[gl_PrimitiveID];

    // // Get the vertices associated with this hit
    // VertexAttribute v0 = AttribsArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].VertexAttribs[int(face.x)];
    // VertexAttribute v1 = AttribsArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].VertexAttribs[int(face.y)];
    // VertexAttribute v2 = AttribsArray[nonuniformEXT(gl_InstanceCustomIndexEXT)].VertexAttribs[int(face.z)];

    // // Using the barycentric coordinates, find the uv for this hit
    // const vec2 uv = BaryLerp(v0.uv.xy, v1.uv.xy, v2.uv.xy, barycentrics);

    // // Return color information for hit for shadowed objects that allow light to pass through
    // vec3 texel = mat.albedo.rgb;
    // if(mat.albedoIndex != -1)
    // {
    //     texel *= textureLod(TexturesArray[nonuniformEXT(mat.albedoIndex)], uv, 0.0f).rgb;
    // }

    // ShadowRay.matId = matId;
    // ShadowRay.distance = gl_HitTEXT;
    // ShadowRay.albedo = vec4(texel, 0.0f);
}
