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

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texture;
layout(location = 3) in flat uint in_model_id;

layout (push_constant) uniform PushConstants {
    mat4 perspective_camera;
    vec3 camera_position;
};

layout(location = 0) out vec4 out_color;

const uint MAX_LIGHTS = 512;
layout(set = 0, binding = 0) uniform lights_uniform {
    vec4 lights[MAX_LIGHTS];
};
layout(set = 0, binding = 1) uniform sampler2D textures[];

float light_intensity(vec4 light) {
    vec3 corrected_normal = normalize(in_normal);
    float light_distance = length(light.xyz - in_position);
    vec3 light_dir = normalize(light.xyz - in_position);
    vec3 view_dir = normalize(camera_position - in_position);
    vec3 reflect_dir = reflect(-light_dir, corrected_normal);

    float ambient = 1.0;
    float diffuse = max(dot(light_dir, corrected_normal), 0.0);
    float specular = pow(max(dot(view_dir, reflect_dir), 0.0), 32);

    return dot(vec3(ambient, diffuse, specular), vec3(0.2, 0.4, 0.4)) / light_distance * light.w;
}

void main() {
    out_color = texture(textures[in_model_id], in_texture) * vec4(vec3(light_intensity(lights[0])), 1.0);
}
