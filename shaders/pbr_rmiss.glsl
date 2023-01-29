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
#pragma shader_stage(miss)
#extension GL_EXT_ray_tracing : require

struct hit_payload {
    vec3 hit_value;
};

layout(location = 0) rayPayloadInEXT hit_payload prd;

layout (push_constant) uniform PushConstants {
    mat4 perspective;
    mat4 camera;
};

void main() {
    prd.hit_value = gl_WorldRayDirectionEXT * 0.5 + 0.5;
}