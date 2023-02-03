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
#extension GL_EXT_ray_tracing : require
#endif
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : require

#ifdef RAY_TRACING
struct hit_payload {
    vec3 albedo;
    vec3 normal;
    float roughness;
    float metallicity;
    vec3 hit_position;
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
};

const uint MAX_LIGHTS = 512;
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

layout(set = 1, binding = 1, rgba8) uniform writeonly image2D image;

#ifdef RAY_TRACING
layout(set = 1, binding = 2, scalar) buffer objects_buf { obj_desc i[]; } objects;
#endif

const float PI = 3.14159265358979;

#ifdef RAY_TRACING
layout(buffer_reference, scalar) buffer vertices_buf {vertex v[]; };
layout(buffer_reference, scalar) buffer indices_buf {uvec3 i[]; };
#endif
