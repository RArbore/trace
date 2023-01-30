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


#version 460
#pragma shader_stage(closest)
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

struct hit_payload {
    vec3 hit_value;
};

struct obj_desc {
    uint64_t vertex_address;
    uint64_t index_address;
    uint64_t model_id;
};

layout(location = 0) rayPayloadInEXT hit_payload prd;

layout (push_constant) uniform PushConstants {
    mat4 perspective;
    mat4 camera;
};

const uint MAX_LIGHTS = 512;
layout(set = 0, binding = 0) uniform lights_uniform {
    vec4 lights[MAX_LIGHTS];
};
layout(set = 0, binding = 1) uniform sampler2D textures[];

const float PI = 3.14159265358979;

void main() {
    prd.hit_value = vec3(1.0, 1.0, 1.0);
}
