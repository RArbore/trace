/*
 * This file is part of trace.
 * trace is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * trace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with trace. If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef RAY_TRACING
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_ray_tracing : require
#endif
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : require

const float PI = 3.14159265358979;
const uint NUM_BOUNCES = 3;
const uint MAX_LIGHTS = 512;
const float SURFACE_OFFSET = 0.002;
const float FLOAT_MAX = 3.402823466e+38;
const float FLOAT_MIN = 1.175494351e-38;

#ifdef RAY_TRACING
struct hit_payload {
    vec3 albedo;
    vec3 normal;
    vec3 flat_normal;
    float roughness;
    float metallicity;
    vec3 hit_position;
    float direct_emittance;
    uint model_id;
};

struct obj_desc {
    uint64_t vertex_address;
    uint64_t index_address;
    uint64_t model_id;
};

struct vertex {
    vec3 position;
    vec3 normal;
    vec2 texcoord;
};
#endif

layout (push_constant) uniform PushConstants {
    mat4 camera;
    uint seed;
    float alpha;
};

layout(set = 0, binding = 0) uniform lights_uniform {
    vec4 lights[MAX_LIGHTS];
};

layout(set = 0, binding = 1) uniform perspective_uniform {
    mat4 perspective;
};

layout(set = 0, binding = 2) uniform sampler2D textures[];

#ifdef RAY_TRACING
layout(set = 1, binding = 0) uniform accelerationStructureEXT tlas;
#endif

#ifdef RAY_TRACING
layout(set = 1, binding = 1, scalar) buffer objects_buf { obj_desc i[]; } objects;
#endif

layout(set = 1, binding = 2, rgba8) uniform readonly image2D blue_noise_image;

layout(set = 1, binding = 3, rgba16f) uniform image2D ray_tracing_albedo_image;
layout(set = 1, binding = 4, rgba16f) uniform image2D ray_tracing_lighting_image;
layout(set = 1, binding = 5, r16f) uniform image2D ray_tracing_depth_image;
layout(set = 1, binding = 6, rgba8) uniform image2D ray_tracing_normal_image;

layout(set = 1, binding = 7, rgba16f) uniform image2D last_frame_albedo_image;
layout(set = 1, binding = 8, rgba16f) uniform image2D last_frame_lighting_image;
layout(set = 1, binding = 9, r16f) uniform image2D last_frame_depth_image;
layout(set = 1, binding = 10, rgba8) uniform image2D last_frame_normal_image;

#ifdef RAY_TRACING
layout(buffer_reference, scalar) buffer vertices_buf {vertex v[]; };
layout(buffer_reference, scalar) buffer indices_buf {uvec3 i[]; };
#endif

uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

float float_construct(uint m) {
    const uint ieeeMantissa = 0x007FFFFFu;
    const uint ieeeOne = 0x3F800000u;

    m &= ieeeMantissa;
    m |= ieeeOne;

    float f = uintBitsToFloat(m);
    return f - 1.0;
}

float random_float(uvec2 coords, uint seed) {
    uint m = hash(coords.x ^ hash(coords.y ^ hash(seed)));
    return float_construct(m);    
}

vec4 random_vec4(uvec2 coords, uint seed) {
    uint m = hash(coords.x ^ hash(coords.y ^ hash(seed)));
    vec4 ret;
    ret.x = float_construct(m);
    m ^= hash(m);
    ret.y = float_construct(m);
    m ^= hash(m);
    ret.z = float_construct(m);
    m ^= hash(m);
    ret.w = float_construct(m);
    return ret;
}
