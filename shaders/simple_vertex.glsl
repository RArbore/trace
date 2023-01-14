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
#pragma shader_stage(vertex)

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_texture;
layout(location = 3) in mat4 in_model;

layout (push_constant) uniform PushConstants {
    mat4 perspective_camera;
    vec3 camera_position;
};

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_texture;
layout(location = 3) out flat uint out_model_id;

void main() {
    gl_Position = perspective_camera * in_model * vec4(in_position, 1.0);
    out_position = (in_model * vec4(in_position, 1.0)).xyz;
    out_normal = (in_model * vec4(in_normal, 0.0)).xyz;
    out_texture = in_texture;
    out_model_id = gl_DrawID;
}
