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

#ifdef RAY_TRACING
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_ray_tracing : require
#endif
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : require

const uint NUM_BOUNCES = 3;
const uint MAX_LIGHTS = 512;
const uint KIND_TRIANGLE = 0;
const uint KIND_VOXEL = 1;
const uint KIND_VOLUMETRIC = 2;
const uint KIND_LIGHT = 3;
const uint KIND_MISS = 4;

const float PI = 3.14159265358979;
const float WEIGHT_CUTOFF = 0.1;
const float LIGHT_RADIUS = 0.5;
const float SURFACE_OFFSET = 0.0001;
const float FLOAT_MAX = 3.402823466e+38;
const float FLOAT_MIN = 1.175494351e-38;
const float FAR_AWAY = 1000.0;

const vec3 voxel_normals[6] = vec3[6](
				      vec3(-1.0, 0.0, 0.0),
				      vec3(1.0, 0.0, 0.0),
				      vec3(0.0, -1.0, 0.0),
				      vec3(0.0, 1.0, 0.0),
				      vec3(0.0, 0.0, -1.0),
				      vec3(0.0, 0.0, 1.0)
				      );

#ifdef RAY_TRACING
struct hit_payload {
    vec3 albedo;
    vec3 normal;
    vec3 flat_normal;

    float roughness;
    float metallicity;
    float direct_emittance;

    vec3 hit_position;

    uint model_kind;
    uint model_id;

    float volumetric_weight;
    vec3 volumetric_front_position;
    vec3 volumetric_back_position;
    float volumetric_dls_weight;
    vec3 volumetric_dls_back_position;
};

struct obj_desc {
    uint64_t vertex_address;
    uint64_t index_address;
    uint64_t model_id;
};

struct vertex {
    vec3 position;
    vec3 normal;
    vec2 texcoord;
};
#endif

struct ray_sample {
    vec3 drawn_sample;
    float drawn_weight;
};

struct aabb_intersect_result {
    float t;
    float back_t;
    uint k;
};

struct pixel_sample {
    vec3 albedo;
    vec3 lighting;
    vec3 position;
    vec3 normal;
    uint model_id;
    float luminance_moment1;
    float luminance_moment2;
    float variance;
    float history_length;
};

layout (push_constant) uniform PushConstants {
    uint current_frame;
    float alpha_temporal;
    float alpha_taa;
    float sigma_normal;
    float sigma_position;
    float sigma_luminance;
    uint filter_iter;
    uint num_filter_iters;
    uint temporal;
    uint taa;
};

layout(set = 0, binding = 0) uniform lights_uniform {
    vec4 lights[MAX_LIGHTS];
};

layout(set = 0, binding = 1) uniform projection_uniform {
    mat4 perspective;
    mat4 inverse_perspective;
    mat4 camera;
    mat4 last_frame_camera;
    mat4 inverse_camera;
    mat4 inverse_last_frame_camera;
    mat4 centered_camera;
    mat4 centered_last_frame_camera;
    mat4 centered_inverse_camera;
    mat4 centered_inverse_last_frame_camera;
    mat4 inverse_jittered_perspective;
    vec3 camera_position;
    vec3 view_dir;
    vec3 basis_right;
    vec3 basis_up;
    vec3 ray_trace2_camera_position;
    vec3 ray_trace2_view_dir;
    vec3 ray_trace2_basis_right;
    vec3 ray_trace2_basis_up;
};

layout(set = 0, binding = 2) uniform sampler2D textures[];

#ifdef RAY_TRACING
layout(set = 1, binding = 0) uniform accelerationStructureEXT tlas;
#endif

#ifdef RAY_TRACING
layout(set = 1, binding = 1, scalar) buffer objects_buf { obj_desc i[]; } objects;
#endif

layout(set = 1, binding = 2, rgba8) uniform readonly image2D blue_noise_image;

layout(set = 1, binding = 3, rgba8) uniform image2D ray_trace1_albedo_image;
layout(set = 1, binding = 4, rgba32f) uniform image2D ray_trace1_lighting1_image;
layout(set = 1, binding = 5, rgba32f) uniform image2D ray_trace1_lighting2_image;
layout(set = 1, binding = 6, rgba16f) uniform image2D ray_trace1_position_image;
layout(set = 1, binding = 7, rgba8) uniform image2D ray_trace1_normal_image;
layout(set = 1, binding = 8, rgba32f) uniform image2D ray_trace1_history1_image;
layout(set = 1, binding = 9, rgba32f) uniform image2D ray_trace1_history2_image;

layout(set = 1, binding = 10, rgba8) uniform image2D ray_trace2_albedo_image;
layout(set = 1, binding = 11, rgba32f) uniform image2D ray_trace2_lighting1_image;
layout(set = 1, binding = 12, rgba32f) uniform image2D ray_trace2_lighting2_image;
layout(set = 1, binding = 13, rgba16f) uniform image2D ray_trace2_position_image;
layout(set = 1, binding = 14, rgba8) uniform image2D ray_trace2_normal_image;
layout(set = 1, binding = 15, rgba32f) uniform image2D ray_trace2_history1_image;
layout(set = 1, binding = 16, rgba32f) uniform image2D ray_trace2_history2_image;

layout(set = 1, binding = 17) uniform sampler2D ray_trace1_albedo_texture;
layout(set = 1, binding = 18) uniform sampler2D ray_trace1_lighting1_texture;
layout(set = 1, binding = 19) uniform sampler2D ray_trace1_lighting2_texture;
layout(set = 1, binding = 20) uniform sampler2D ray_trace1_position_texture;
layout(set = 1, binding = 21) uniform sampler2D ray_trace1_normal_texture;
layout(set = 1, binding = 22) uniform sampler2D ray_trace1_history1_texture;
layout(set = 1, binding = 23) uniform sampler2D ray_trace1_history2_texture;

layout(set = 1, binding = 24) uniform sampler2D ray_trace2_albedo_texture;
layout(set = 1, binding = 25) uniform sampler2D ray_trace2_lighting1_texture;
layout(set = 1, binding = 26) uniform sampler2D ray_trace2_lighting2_texture;
layout(set = 1, binding = 27) uniform sampler2D ray_trace2_position_texture;
layout(set = 1, binding = 28) uniform sampler2D ray_trace2_normal_texture;
layout(set = 1, binding = 29) uniform sampler2D ray_trace2_history1_texture;
layout(set = 1, binding = 30) uniform sampler2D ray_trace2_history2_texture;

layout(set = 1, binding = 31, rg32f) uniform image2D motion_vector_image;
layout(set = 1, binding = 32) uniform sampler2D motion_vector_texture;

layout(set = 1, binding = 33, rgba32f) uniform image2D taa1_image;
layout(set = 1, binding = 34, rgba32f) uniform image2D taa2_image;
layout(set = 1, binding = 35) uniform sampler2D taa1_texture;
layout(set = 1, binding = 36) uniform sampler2D taa2_texture;

layout(set = 1, binding = 37) buffer palette_buf { uint p[]; };

layout(set = 1, binding = 38, r8) uniform readonly image3D volumes[];

#ifdef RAY_TRACING
layout(buffer_reference, scalar) buffer vertices_buf { vertex v[]; };
layout(buffer_reference, scalar) buffer indices_buf { uvec3 i[]; };
#endif

uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

float float_construct(uint m) {
    const uint ieeeMantissa = 0x007FFFFFu;
    const uint ieeeOne = 0x3F800000u;

    m &= ieeeMantissa;
    m |= ieeeOne;

    float f = uintBitsToFloat(m);
    return f - 1.0;
}

float random_float(uvec2 coords, uint seed) {
    uint m = hash(coords.x ^ hash(coords.y ^ hash(seed)));
    return float_construct(m);    
}

vec4 random_vec4(uvec2 coords, uint seed) {
    uint m = hash(coords.x ^ hash(coords.y ^ hash(seed)));
    vec4 ret;
    ret.x = float_construct(m);
    m ^= hash(m);
    ret.y = float_construct(m);
    m ^= hash(m);
    ret.z = float_construct(m);
    m ^= hash(m);
    ret.w = float_construct(m);
    return ret;
}

vec2 pixel_coord_to_device_coord(vec2 pixel_coord) {
    vec2 in_UV = pixel_coord / vec2(imageSize(ray_trace1_albedo_image));
    return in_UV * 2.0 - 1.0;
}

vec2 device_coord_to_pixel_coord(vec2 device_coord) {
    vec2 in_UV = device_coord * 0.5 + 0.5;
    return in_UV * vec2(imageSize(ray_trace1_albedo_image));
}

pixel_sample get_new_sample(vec2 pixel_coord) {
    if (current_frame % 2 == 0) {
	vec2 uv = pixel_coord_to_device_coord(pixel_coord) * 0.5 + 0.5;
	pixel_sample s;
	vec4 albedo_sample = texture(ray_trace1_albedo_texture, uv);
	s.albedo = albedo_sample.xyz;
	s.model_id = uint(albedo_sample.w * 255.0);
	vec4 history;
	if (filter_iter % 2 == 0) {
	    s.lighting = texture(ray_trace1_lighting1_texture, uv).xyz;
	    history = texture(ray_trace1_history1_texture, uv);
	} else {
	    s.lighting = texture(ray_trace1_lighting2_texture, uv).xyz;
	    history = texture(ray_trace1_history2_texture, uv);
	}
	s.position = texture(ray_trace1_position_texture, uv).xyz;
	s.normal = normalize(texture(ray_trace1_normal_texture, uv).xyz * 2.0 - 1.0);
	s.luminance_moment1 = history.x;
	s.luminance_moment2 = history.y;
	s.variance = history.z;
	s.history_length = history.w;
	return s;
    } else {
	vec2 uv = pixel_coord_to_device_coord(pixel_coord) * 0.5 + 0.5;
	pixel_sample s;
	vec4 albedo_sample = texture(ray_trace2_albedo_texture, uv);
	s.albedo = albedo_sample.xyz;
	s.model_id = uint(albedo_sample.w * 255.0);
	vec4 history;
	if (filter_iter % 2 == 0) {
	    s.lighting = texture(ray_trace2_lighting1_texture, uv).xyz;
	    history = texture(ray_trace2_history1_texture, uv);
	} else {
	    s.lighting = texture(ray_trace2_lighting2_texture, uv).xyz;
	    history = texture(ray_trace2_history2_texture, uv);
	}
	s.position = texture(ray_trace2_position_texture, uv).xyz;
	s.normal = normalize(texture(ray_trace2_normal_texture, uv).xyz * 2.0 - 1.0);
	s.luminance_moment1 = history.x;
	s.luminance_moment2 = history.y;
	s.variance = history.z;
	s.history_length = history.w;
	return s;
    }
}

pixel_sample get_old_sample(vec2 pixel_coord, uint old_filter_iter) {
    if (current_frame % 2 == 1) {
	vec2 uv = pixel_coord_to_device_coord(pixel_coord) * 0.5 + 0.5;
	pixel_sample s;
	vec4 albedo_sample = texture(ray_trace1_albedo_texture, uv);
	s.albedo = albedo_sample.xyz;
	s.model_id = uint(albedo_sample.w * 255.0);
	vec4 history;
	if (old_filter_iter % 2 == 1) {
	    s.lighting = texture(ray_trace1_lighting1_texture, uv).xyz;
	    history = texture(ray_trace1_history1_texture, uv);
	} else {
	    s.lighting = texture(ray_trace1_lighting2_texture, uv).xyz;
	    history = texture(ray_trace1_history2_texture, uv);
	}
	s.position = texture(ray_trace1_position_texture, uv).xyz;
	s.normal = normalize(texture(ray_trace1_normal_texture, uv).xyz * 2.0 - 1.0);
	s.luminance_moment1 = history.x;
	s.luminance_moment2 = history.y;
	s.variance = history.z;
	s.history_length = history.w;
	return s;
    } else {
	vec2 uv = pixel_coord_to_device_coord(pixel_coord) * 0.5 + 0.5;
	pixel_sample s;
	vec4 albedo_sample = texture(ray_trace2_albedo_texture, uv);
	s.albedo = albedo_sample.xyz;
	s.model_id = uint(albedo_sample.w * 255.0);
	vec4 history;
	if (old_filter_iter % 2 == 1) {
	    s.lighting = texture(ray_trace2_lighting1_texture, uv).xyz;
	    history = texture(ray_trace2_history1_texture, uv);
	} else {
	    s.lighting = texture(ray_trace2_lighting2_texture, uv).xyz;
	    history = texture(ray_trace2_history2_texture, uv);
	}
	s.position = texture(ray_trace2_position_texture, uv).xyz;
	s.normal = normalize(texture(ray_trace2_normal_texture, uv).xyz * 2.0 - 1.0);
	s.luminance_moment1 = history.x;
	s.luminance_moment2 = history.y;
	s.variance = history.z;
	s.history_length = history.w;
	return s;
    }
}

vec3 get_new_lighting(vec2 pixel_coord) {
    if (current_frame % 2 == 0) {
	vec2 uv = pixel_coord_to_device_coord(pixel_coord) * 0.5 + 0.5;
	if (filter_iter % 2 == 0) {
	    return texture(ray_trace1_lighting1_texture, uv).xyz;
	} else {
	    return texture(ray_trace1_lighting2_texture, uv).xyz;
	}
    } else {
	vec2 uv = pixel_coord_to_device_coord(pixel_coord) * 0.5 + 0.5;
	if (filter_iter % 2 == 0) {
	    return texture(ray_trace2_lighting1_texture, uv).xyz;
	} else {
	    return texture(ray_trace2_lighting2_texture, uv).xyz;
	}
    }
}

void set_new_lighting(vec3 lighting, vec2 pixel_coord) {
    if (current_frame % 2 == 0) {
	if (filter_iter % 2 == 1) {
	    imageStore(ray_trace1_lighting1_image, ivec2(pixel_coord), vec4(lighting, 1.0));
	} else {
	    imageStore(ray_trace1_lighting2_image, ivec2(pixel_coord), vec4(lighting, 1.0));
	}
    } else {
	if (filter_iter % 2 == 1) {
	    imageStore(ray_trace2_lighting1_image, ivec2(pixel_coord), vec4(lighting, 1.0));
	} else {
	    imageStore(ray_trace2_lighting2_image, ivec2(pixel_coord), vec4(lighting, 1.0));
	}
    }
}

void set_new_history(pixel_sample s, vec2 pixel_coord) {
    vec4 history = vec4(s.luminance_moment1, s.luminance_moment2, s.variance, s.history_length);
    if (current_frame % 2 == 0) {
	if (filter_iter % 2 == 1) {
	    imageStore(ray_trace1_history1_image, ivec2(pixel_coord), history);
	} else {
	    imageStore(ray_trace1_history2_image, ivec2(pixel_coord), history);
	}
    } else {
	if (filter_iter % 2 == 1) {
	    imageStore(ray_trace2_history1_image, ivec2(pixel_coord), history);
	} else {
	    imageStore(ray_trace2_history2_image, ivec2(pixel_coord), history);
	}
    }
}

vec4 sample_to_color(pixel_sample s) {
    return vec4(s.albedo * s.lighting, 1.0);
}

float luminance(vec3 radiance) {
    return dot(radiance, vec3(0.2125, 0.7154, 0.0721));
}

aabb_intersect_result hit_aabb(const vec3 minimum, const vec3 maximum, const vec3 origin, const vec3 direction) {
    aabb_intersect_result r;
    vec3 invDir = 1.0 / direction;
    vec3 tbot = invDir * (minimum - origin);
    vec3 ttop = invDir * (maximum - origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    float t0 = max(tmin.x, max(tmin.y, tmin.z));
    float t1 = min(tmax.x, min(tmax.y, tmax.z));
    r.t = t1 > max(t0, 0.0) ? t0 : -FAR_AWAY;
    r.back_t = t1 > max(t0, 0.0) ? t1 : -FAR_AWAY;
    if (t0 == tmin.x) {
	r.k = tbot.x > ttop.x ? 1 : 0;
    } else if (t0 == tmin.y) {
	r.k = tbot.y > ttop.y ? 3 : 2;
    } else if (t0 == tmin.z) {
	r.k = tbot.z > ttop.z ? 5 : 4;
    }
    return r;
}

#ifdef RAY_TRACING
hit_payload create_miss(vec3 origin, vec3 direction) {
    hit_payload prd;
    prd.albedo = vec3(1.0);
    prd.normal = vec3(0.0);
    prd.flat_normal = vec3(0.0, 0.0, 1.0);
    prd.roughness = 0.0;
    prd.metallicity = 0.0;
    prd.hit_position = origin + direction * FAR_AWAY;
    prd.direct_emittance = 0.3;
    prd.model_kind = KIND_MISS;
    prd.model_id = 0xFFFFFFFF;
    return prd;
}
#endif
