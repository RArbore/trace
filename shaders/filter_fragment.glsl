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
    float depth;
    vec3 normal;
};

pixel_sample get_new_sample() {
    ivec2 pixel_coord = ivec2(gl_FragCoord.xy);
    pixel_sample s;
    s.albedo = imageLoad(ray_tracing_albedo_image, pixel_coord).xyz;
    s.lighting = imageLoad(ray_tracing_lighting_image, pixel_coord).xyz;
    s.depth = imageLoad(ray_tracing_depth_image, pixel_coord).x;
    s.normal = imageLoad(ray_tracing_normal_image, pixel_coord).xyz * 2.0 - 1.0;
    return s;
}

pixel_sample get_old_sample() {
    ivec2 pixel_coord = ivec2(gl_FragCoord.xy);
    pixel_sample s;
    s.albedo = imageLoad(last_frame_albedo_image, pixel_coord).xyz;
    s.lighting = imageLoad(last_frame_lighting_image, pixel_coord).xyz;
    s.depth = imageLoad(last_frame_depth_image, pixel_coord).x;
    s.normal = imageLoad(last_frame_normal_image, pixel_coord).xyz * 2.0 - 1.0;
    return s;
}

void set_sample(pixel_sample s) {
    ivec2 pixel_coord = ivec2(gl_FragCoord.xy);
    imageStore(last_frame_albedo_image, pixel_coord, vec4(s.albedo, 1.0));
    imageStore(last_frame_lighting_image, pixel_coord, vec4(s.lighting, 1.0));
    imageStore(last_frame_depth_image, pixel_coord, vec4(s.depth));
    imageStore(last_frame_normal_image, pixel_coord, vec4(s.normal * 0.5 + 0.5, 1.0));
}

vec4 sample_to_color(pixel_sample s) {
    return vec4(s.albedo * s.lighting, 1.0);
}

void main() {
    ivec2 pixel_coord = ivec2(gl_FragCoord.xy);
    pixel_sample new_sample = get_new_sample();
    pixel_sample old_sample = get_old_sample();

    vec3 blended_lighting = mix(old_sample.lighting, new_sample.lighting, alpha);
    pixel_sample blended_sample = new_sample;
    blended_sample.lighting = blended_lighting;
    
    set_sample(blended_sample);
    out_color = sample_to_color(blended_sample);
}
