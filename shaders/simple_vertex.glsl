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
layout(location = 1) in vec3 in_color;

layout (push_constant) uniform PushConstants {
    mat4 perspective_camera;
} push;

layout(location = 0) out vec3 out_color;

void main() {
    gl_Position = push.perspective_camera * vec4(in_position, 1.0);
    out_color = in_color;
}
