// File to share structures between Vulkan C++ code and GLSL

#ifndef SHADER_CONSTANTS_H
#define SHADER_CONSTANTS_H

#define DESCRIPTOR_SET_SIZE                       8

// Descriptor set bindings
#define DESCRIPTOR_SET_ACCELERATION_STRUCTURE     0
#define DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE 0

#define DESCRIPTOR_SET_SCENE_DATA                 0
#define DESCRIPTOR_BINDING_SCENE_DATA             1

#define DESCRIPTOR_SET_CAMERA_DATA                0
#define DESCRIPTOR_BINDING_CAMERA_DATA            2

#define DESCRIPTOR_SET_RENDER_TARGET              1
#define DESCRIPTOR_BINDING_RENDER_TARGET          0

#define DESCRIPTOR_SET_FACE_DATA                  2
#define DESCRIPTOR_BINDING_FACE_DATA              0

#define DESCRIPTOR_SET_VERTEX_ATTRIBUTES          3
#define DESCRIPTOR_BINDING_VERTEX_ATTRIBUTES      0

#define DESCRIPTOR_SET_LIGHTS_DATA                4
#define DESCRIPTOR_BINDING_LIGHTS_DATA            0

#define DESCRIPTOR_SET_MATERIALS_DATA             5
#define DESCRIPTOR_BINDING_MATERIALS_DATA         0

#define DESCRIPTOR_SET_TEXTURES_DATA              6
#define DESCRIPTOR_BINDING_TEXTURES_DATA          0

#define DESCRIPTOR_SET_INSTANCE_DATA              7
#define DESCRIPTOR_BINDING_INSTANCE_DATA          0

#define PRIMARY_HIT_SHADERS_INDEX   0
#define PRIMARY_MISS_SHADERS_INDEX  0
#define SHADOW_HIT_SHADERS_INDEX    1
#define SHADOW_MISS_SHADERS_INDEX   1


#define LOCATION_PRIMARY_RAY    0
#define LOCATION_SHADOW_RAY     1

#define RAYTRACE_MAX_RECURSION 10

#ifdef __cplusplus

#define align4  alignas(4)
#define align8  alignas(8)
#define align16 alignas(16)
#define align64 alignas(64)

#else

#define align4
#define align8
#define align16
#define align64 

#endif

struct ShaderRayPayload {
    align16 vec4  albedo;
    align16 vec4  emission;
    align4  float metallic;
    align4  float roughness;
    align4  float ambientOcclusion;
    align4  float indexOfRefraction;

    align16 vec3  normal;
    align4  float distance;
#ifdef __cplusplus
    align4 uint32_t materialIndex;
#else
    align4 uint     materialIndex;
#endif
};

struct ShaderShadowRayPayload {
    align16 vec4 albedo;
    align4 float distance;
#ifdef __cplusplus
    align4 uint32_t materialIndex;
#else
    align4 uint     materialIndex;
#endif
};

// Ability to build tri face in hit shader
struct ShaderFaceData {
#ifdef __cplusplus
    align4  uint32_t index0;
    align4  uint32_t index1;
    align4  uint32_t index2;
#else
    align4  uint     index0;
    align4  uint     index1;
    align4  uint     index2;
#endif
};


// Ability to resolve vertex in hit shader
struct ShaderVertexAttributeData {
    align16 vec3        normal;
    align16 vec4        tangent;
    align8  vec2        uv;
};

struct ShaderInstanceData {
    align64 mat4        localToWorld;
    align64 mat4        worldToLocal;
#ifdef __cplusplus
    align4  uint32_t    materialIndex;    // This is the mapping done during descriptor update
#else
    align4  uint        materialIndex;    // This is the mapping done during descriptor update
#endif
};


// packed std140
struct ShaderSceneData {
    align16 vec4        ambient;
#ifdef __cplusplus
    align4  uint32_t    lightCount;
#else
    align4  uint        lightCount;
#endif
};

#define SHADER_LIGHTTYPE_NONE        -1
#define SHADER_LIGHTTYPE_SPOT         0
#define SHADER_LIGHTTYPE_DIRECTIONAL  1
#define SHADER_LIGHTTYPE_POINT        2
#define SHADER_LIGHTTYPE_AREA         3

// packed std140
struct ShaderLightingData {
#ifdef __cplusplus
    align4  int32_t lightInstanceId;
    align4  int32_t type;
#else
    align4  int     lightInstanceId;
    align4  int     type;
#endif
    align16 vec3     position;
    align16 vec3     color;
    align4  float    bounceIntensity;
    align4  float    intensity;
    align4  float    range;
    align4  float    spotAngle;
    align4  bool     enabled;
};

// packed std140
struct ShaderCameraData {
    // Camera
    align16 vec4  position;
    align16 vec4  direction;
    align16 vec4  up;
    align16 vec4  right;
    align4  float nearClipPlane;
    align4  float farClipPlane;
    align4  float fieldOfView;
};

// packed std140
struct ShaderMaterialData {
    align16 vec4  albedo;
    align16 vec4  emission;
    align4  float metallic;
    align4  float roughness;
    align4  float indexOfRefraction;
    align16 vec4  transmittance;
    align4  bool  albedoTextureSet;
    align4  bool  emissionTextureSet;
    align4  bool  normalTextureSet;
    align4  bool  metallicTextureSet;
    align4  bool  roughnessTextureSet;
    align4  bool  ambientOcclusionTextureSet;
#ifdef __cplusplus
    align4  int32_t  albedoTextureInstanceId;
    align4  int32_t  emissionTextureInstanceId;
    align4  int32_t  normalTextureInstanceId;
    align4  int32_t  metallicTextureInstanceId;
    align4  int32_t  roughnessTextureInstanceId;
    align4  int32_t  ambientOcclusionTextureInstanceId;
    align4  uint32_t albedoTextureIndex;
    align4  uint32_t emissionTextureIndex;
    align4  uint32_t normalTextureIndex;
    align4  uint32_t metallicTextureIndex;
    align4  uint32_t roughnessTextureIndex;
    align4  uint32_t ambientOcclusionTextureIndex;
#else
    align4  int  albedoTextureInstanceId;
    align4  int  emissionTextureInstanceId;
    align4  int  normalTextureInstanceId;
    align4  int  metallicTextureInstanceId;
    align4  int  roughnessTextureInstanceId;
    align4  int  ambientOcclusionTextureInstanceId;
    align4  uint albedoTextureIndex;
    align4  uint emissionTextureIndex;
    align4  uint normalTextureIndex;
    align4  uint metallicTextureIndex;
    align4  uint roughnessTextureIndex;
    align4  uint ambientOcclusionTextureIndex;
#endif
};

// shaders helper functions
vec2 BaryLerp(vec2 a, vec2 b, vec2 c, vec3 barycentrics) {
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

vec3 BaryLerp(vec3 a, vec3 b, vec3 c, vec3 barycentrics) {
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

vec4 BaryLerp(vec4 a, vec4 b, vec4 c, vec3 barycentrics) {
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

float LinearToSrgb(float channel) {
    if (channel <= 0.0031308f) {
        return 12.92f * channel;
    }
    else {
        return 1.055f * pow(channel, 1.0f / 2.4f) - 0.055f;
    }
}

vec3 LinearToSrgb(vec3 linear) {
    return vec3(LinearToSrgb(linear.r), LinearToSrgb(linear.g), LinearToSrgb(linear.b));
}

#endif // SHADER_CONSTANTS_H
