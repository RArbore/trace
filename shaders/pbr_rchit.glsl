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
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable

struct hit_payload {
    vec3 hit_value;
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
layout(set = 1, binding = 2, scalar) buffer objects_buf { obj_desc i[]; } objects;

layout(buffer_reference, scalar) buffer vertices_buf {vertex v[]; };
layout(buffer_reference, scalar) buffer indices_buf {uvec3 i[]; };

hitAttributeEXT vec2 attribs;

const float PI = 3.14159265358979;

void main() {
    obj_desc obj = objects.i[gl_InstanceCustomIndexEXT];
    vertices_buf vertices = vertices_buf(obj.vertex_address);
    indices_buf indices = indices_buf(obj.index_address);

    uvec3 index = indices.i[gl_PrimitiveID];

    vertex v0 = vertices.v[index.x];
    vertex v1 = vertices.v[index.y];
    vertex v2 = vertices.v[index.z];

    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    vec3 obj_position = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
    vec3 world_position = vec3(gl_ObjectToWorldEXT * vec4(obj_position, 1.0));

    vec3 normal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
    vec3 world_normal = normalize(vec3(normal * gl_WorldToObjectEXT));

    vec2 texcoord = v0.texcoord * barycentrics.x + v1.texcoord * barycentrics.y + v2.texcoord * barycentrics.z;
  
    uint texture_base_id = uint(obj.model_id) * 4;
    prd.hit_value = texture(textures[texture_base_id], texcoord).xyz;
}
