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
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

const vec2 positions[6] = vec2[](
				 vec2(-1.0, -1.0),
				 vec2(1.0, -1.0),
				 vec2(-1.0, 1.0),
				 vec2(-1.0, 1.0),
				 vec2(1.0, -1.0),
				 vec2(1.0, 1.0)
				 );

layout(location = 0) out vec2 out_position;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    out_position = positions[gl_VertexIndex];
}
