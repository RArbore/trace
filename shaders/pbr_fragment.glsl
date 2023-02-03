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
layout(location = 3) in flat uint in_model_info;

layout (push_constant) uniform PushConstants {
    mat4 camera;
};

layout(location = 0) out vec4 out_color;

const uint MAX_LIGHTS = 512;
layout(set = 0, binding = 0) uniform lights_uniform {
    vec4 lights[MAX_LIGHTS];
};
layout(set = 0, binding = 2) uniform sampler2D textures[];

const float PI = 3.14159265358979;

float normal_distribution(vec3 normal, vec3 halfway, float alpha) {
    float alpha_2 = alpha * alpha;
    float normal_dot_halfway = max(dot(normal, halfway), 0.0);
    float normal_dot_halfway_2 = normal_dot_halfway * normal_dot_halfway;
    
    float nominator = alpha_2;
    float denominator = (normal_dot_halfway_2 * (alpha_2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;
    
    return nominator / denominator;
}

float geometry_schlick(float normal_dot_view, float k) {
    float nominator = normal_dot_view;
    float denominator = normal_dot_view * (1.0 - k) + k;
	
    return nominator / denominator;
}
  
float geometry_smith(vec3 normal, vec3 view, vec3 light, float k) {
    float normal_dot_view = max(dot(normal, view), 0.0);
    float normal_dot_light = max(dot(normal, light), 0.0);
    float ggx1 = geometry_schlick(normal_dot_view, k);
    float ggx2 = geometry_schlick(normal_dot_light, k);
	
    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cos_theta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

void main() {
    uint texture_id = in_model_info & 0xFFFF;
    vec3 albedo = texture(textures[texture_id], in_texture).xyz;
    vec3 normal = texture(textures[texture_id + 1], in_texture).xyz;
    float roughness = texture(textures[texture_id + 2], in_texture).x;
    float metallicity = texture(textures[texture_id + 3], in_texture).x;

    vec3 tangent_normal = normal * 2.0 - 1.0;

    vec3 Q1 = dFdx(in_position);
    vec3 Q2 = dFdy(in_position);
    vec2 st1 = dFdx(in_texture);
    vec2 st2 = dFdy(in_texture);

    vec3 N = normalize(in_normal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    vec3 corrected_normal = normalize(TBN * tangent_normal);

    vec3 cam_pos = (inverse(camera) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 view_dir = normalize(cam_pos - in_position);
    vec3 F0_dieletric = vec3(0.04); 
    vec3 F0 = mix(F0_dieletric, albedo, metallicity);
    float alpha = roughness * roughness;
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;

    uint num_lights = floatBitsToUint(lights[0].x);
    vec3 outward_radiance = vec3(0.0);
    for (uint light_id = 0; light_id < num_lights; ++light_id) {
	vec3 light_position = lights[light_id + 1].xyz;
	float light_intensity = lights[light_id + 1].w;

	vec3 light_dir = normalize(light_position - in_position);
	vec3 halfway_dir = normalize(view_dir + light_dir);

	float distance = length(light_position - in_position);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = vec3(light_intensity) * attenuation; 

	float D = normal_distribution(corrected_normal, halfway_dir, alpha);
	float G = geometry_smith(corrected_normal, view_dir, light_dir, k);
	vec3 F = fresnel_schlick(clamp(dot(halfway_dir, view_dir), 0.0, 1.0), F0);

	vec3 numerator = D * G * F;
	float denominator = 4.0 * max(dot(corrected_normal, view_dir), 0.0) * max(dot(corrected_normal, light_dir), 0.0)  + 0.0001;
	vec3 specular = numerator / denominator;

	vec3 kS = F;
	vec3 kD = (vec3(1.0) - kS) * (1.0 - metallicity);
	float normal_dot_light = max(dot(corrected_normal, light_dir), 0.0);
	outward_radiance += (kD * albedo / PI + specular) * radiance * normal_dot_light;
    }

    vec3 color = outward_radiance + vec3(0.03) * albedo;
    out_color = vec4(color, 1.0);
}
