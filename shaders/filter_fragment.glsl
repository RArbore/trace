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
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) out vec4 out_color;

struct pixel_sample {
    vec3 albedo;
    vec3 lighting;
    vec3 position;
    vec3 normal;
};

pixel_sample get_new_sample(ivec2 pixel_coord) {
    pixel_sample s;
    s.albedo = imageLoad(ray_tracing_albedo_image, pixel_coord).xyz;
    s.lighting = imageLoad(ray_tracing_lighting_image, pixel_coord).xyz;
    s.position = imageLoad(ray_tracing_position_image, pixel_coord).xyz;
    s.normal = imageLoad(ray_tracing_normal_image, pixel_coord).xyz * 2.0 - 1.0;
    return s;
}

pixel_sample get_old_sample(ivec2 pixel_coord) {
    pixel_sample s;
    s.albedo = imageLoad(last_frame_albedo_image, pixel_coord).xyz;
    s.lighting = imageLoad(last_frame_lighting_image, pixel_coord).xyz;
    s.position = imageLoad(last_frame_position_image, pixel_coord).xyz;
    s.normal = imageLoad(last_frame_normal_image, pixel_coord).xyz * 2.0 - 1.0;
    return s;
}

void set_sample(pixel_sample s, ivec2 pixel_coord) {
    imageStore(last_frame_albedo_image, pixel_coord, vec4(s.albedo, 1.0));
    imageStore(last_frame_lighting_image, pixel_coord, vec4(s.lighting, 1.0));
    imageStore(last_frame_position_image, pixel_coord, vec4(s.position, 1.0));
    imageStore(last_frame_normal_image, pixel_coord, vec4(s.normal * 0.5 + 0.5, 1.0));
}

vec4 sample_to_color(pixel_sample s) {
    return vec4(s.albedo * s.lighting, 1.0);
}

vec2 pixel_coord_to_device_coord(ivec2 pixel_coord) {
    vec2 pixel_center = vec2(pixel_coord) + vec2(0.5);
    vec2 in_UV = pixel_center / vec2(imageSize(ray_tracing_albedo_image));
    return in_UV * 2.0 - 1.0;
}

ivec2 device_coord_to_pixel_coord(vec2 device_coord) {
    vec2 in_UV = device_coord * 0.5 + 0.5;
    vec2 pixel_center = in_UV * vec2(imageSize(ray_tracing_albedo_image));
    return ivec2(pixel_center - vec2(0.5));
}

void main() {
    ivec2 pixel_coord = ivec2(gl_FragCoord.xy);
    pixel_sample new_sample = get_new_sample(pixel_coord);
    pixel_sample old_sample = get_old_sample(pixel_coord);

    vec4 new_NDC = perspective * camera * vec4(new_sample.position, 1.0);
    vec4 old_NDC = perspective * last_frame_camera * vec4(new_sample.position, 1.0);
    vec3 new_device = new_NDC.xyz / new_NDC.w;
    vec3 old_device = old_NDC.xyz / old_NDC.w;
    vec2 device_velocity = new_device.xy - old_device.xy;

    ivec2 reprojected_pixel_coord = device_coord_to_pixel_coord(pixel_coord_to_device_coord(pixel_coord) - device_velocity);
    pixel_sample reprojected_sample = get_old_sample(reprojected_pixel_coord);

    float depth = length(new_sample.position - camera_position);
    bool blend = depth < 0.8 * FAR_AWAY && camera == last_frame_camera;
    vec3 blended_lighting = mix(old_sample.lighting, new_sample.lighting, alpha);
    pixel_sample blended_sample = new_sample;
    blended_sample.lighting = blend ? blended_lighting : new_sample.lighting;
    
    set_sample(blended_sample, pixel_coord);
    out_color = sample_to_color(blended_sample);
}
