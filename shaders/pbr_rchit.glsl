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
#extension GL_GOOGLE_include_directive : enable

#define RAY_TRACING
#include "common.glsl"

layout(location = 0) rayPayloadInEXT hit_payload prd;

hitAttributeEXT vec2 attribs;

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

    vec3 obj_flat_normal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
    vec3 world_flat_normal = normalize(vec3(obj_flat_normal * gl_WorldToObjectEXT));

    vec2 texcoord = v0.texcoord * barycentrics.x + v1.texcoord * barycentrics.y + v2.texcoord * barycentrics.z;
  
    uint texture_base_id = uint(obj.model_id) * 4;
    vec3 albedo = texture(textures[texture_base_id], texcoord).xyz;
    vec3 bump_normal = texture(textures[texture_base_id + 1], texcoord).xyz;
    float roughness = texture(textures[texture_base_id + 2], texcoord).x;
    float metallicity = texture(textures[texture_base_id + 3], texcoord).x;

    vec3 N = world_flat_normal;
    vec3 triangle_edge1 = v1.position - v0.position;
    vec3 triangle_edge2 = v2.position - v0.position;
    vec2 delta_texcoord1 = v1.texcoord - v0.texcoord;
    vec2 delta_texcoord2 = v2.texcoord - v0.texcoord;
    float det = 1.0 / (delta_texcoord1.x * delta_texcoord2.y - delta_texcoord2.x * delta_texcoord1.y);
    vec3 obj_T = vec3(
		      det * (delta_texcoord2.y * triangle_edge1.x - delta_texcoord1.y * triangle_edge2.x),
		      det * (delta_texcoord2.y * triangle_edge1.y - delta_texcoord1.y * triangle_edge2.y),
		      det * (delta_texcoord2.y * triangle_edge1.z - delta_texcoord1.y * triangle_edge2.z)
		      );
    vec3 T = normalize(vec3(obj_T * gl_WorldToObjectEXT));
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);

    vec3 normal = normalize(TBN * (bump_normal * 2.0 - 1.0));

    prd.albedo = albedo;
    prd.normal = normal;
    prd.flat_normal = world_flat_normal;
    prd.roughness = roughness;
    prd.metallicity = metallicity;
    prd.hit_position = world_position;
    prd.direct_emittance = 0.0;
    prd.model_kind = KIND_TRIANGLE;
    prd.model_id = uint(obj.model_id);
}
