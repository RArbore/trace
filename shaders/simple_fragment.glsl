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
#pragma shader_stage(fragment)
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_texture;
layout(location = 2) in flat uint in_model_id;

layout(location = 0) out vec4 out_color;

const uint MAX_LIGHTS = 512;
layout(set = 0, binding = 0) uniform lights {
    vec4 light[MAX_LIGHTS];
};
layout(set = 0, binding = 1) uniform sampler2D textures[];

void main() {
    //out_color = texture(textures[in_model_id], in_texture);
    vec3 corrected_normal = normalize(in_normal);
    out_color = vec4(corrected_normal / 2.0 + 0.5, 1.0);
}
