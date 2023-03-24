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

#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <array>
#include <bit>
#include <map>

#include "model.h"
#include "util.h"

struct Scene {
    static const uint32_t MAX_LIGHTS = 512;

    struct RayTraceObject {
	uint64_t vertex_address;
	uint64_t index_address;
	uint64_t model_id;
    };
    
    std::vector<Model> models;
    std::vector<std::vector<glm::mat4>> transforms;
    std::vector<std::pair<Image, VkImageView>> textures;
    std::vector<glm::vec4> lights;
    std::vector<VoxelModel> voxel_models;
    std::vector<std::pair<Volume, VkImageView>> voxel_volumes;
    std::vector<std::vector<glm::mat4>> voxel_transforms;
    uint16_t num_models;
    uint32_t num_objects;
    uint16_t num_textures;
    uint16_t num_lights;
    uint16_t num_voxel_models;
    uint32_t num_voxel_objects;

    Buffer vertices_buf, indices_buf, instances_buf, indirect_draw_buf, lights_buf, ray_trace_objects_buf, voxel_palette_buf, light_aabbs_buf;
    std::size_t vertices_buf_contents_size, indices_buf_contents_size, instances_buf_contents_size, indirect_draw_buf_contents_size, lights_buf_contents_size, ray_trace_objects_buf_contents_size, voxel_palette_buf_contents_size, light_aabbs_buf_contents_size;
    std::vector<std::size_t> model_vertices_offsets, model_indices_offsets;
    std::map<std::string, uint16_t> loaded_models;
    std::map<std::string, uint16_t> loaded_voxel_models;

    VkAccelerationStructureKHR tlas;
    std::vector<VkAccelerationStructureKHR> blass;
    std::vector<VkAccelerationStructureKHR> voxel_blass;
    VkAccelerationStructureKHR lights_blas;
    Buffer tlas_buffer, tlas_instances_buffer;
    std::vector<Buffer> blas_buffers;
    std::vector<Buffer> voxel_blas_buffers;
    Buffer lights_blas_buffer;

    auto add_object(const glm::mat4 &&transform, uint16_t model_id) noexcept -> void {
	transforms[model_id].emplace_back(transform);
	uint32_t model_info = models[model_id].base_texture_id;
	transforms[model_id].back()[3][3] = std::bit_cast<float>(model_info);
	++num_objects;
    }

    auto add_voxel_object(const glm::mat4 &&transform, uint16_t voxel_model_id) noexcept -> void {
	voxel_transforms[voxel_model_id].emplace_back(transform);
	transforms[voxel_model_id].back()[3][3] = std::bit_cast<float>((uint32_t) voxel_model_id);
	++num_voxel_objects;
    }

    auto add_light(const glm::vec4 &&light) noexcept -> uint32_t {
	ASSERT(num_lights < MAX_LIGHTS, "Tried to add too many lights.");
	lights.emplace_back(light);
	++num_lights;
	return num_lights - 1;
    }

    static auto binding_descriptions() noexcept -> std::array<VkVertexInputBindingDescription, 2> {
	std::array<VkVertexInputBindingDescription, 2> binding_descriptions {};
	binding_descriptions[0].binding = 0;
	binding_descriptions[0].stride = sizeof(Model::Vertex);
	binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	binding_descriptions[1].binding = 1;
	binding_descriptions[1].stride = sizeof(glm::mat4);
	binding_descriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

	return binding_descriptions;
    }

    static auto attribute_descriptions() noexcept -> std::array<VkVertexInputAttributeDescription, 7> {
	std::array<VkVertexInputAttributeDescription, 7> attribute_descriptions {};
	attribute_descriptions[0].binding = 0;
	attribute_descriptions[0].location = 0;
	attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribute_descriptions[0].offset = offsetof(Model::Vertex, position);
	attribute_descriptions[1].binding = 0;
	attribute_descriptions[1].location = 1;
	attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribute_descriptions[1].offset = offsetof(Model::Vertex, normal);
	attribute_descriptions[2].binding = 0;
	attribute_descriptions[2].location = 2;
	attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attribute_descriptions[2].offset = offsetof(Model::Vertex, texture);

	for (uint32_t i = 0; i < 4; ++i) {
	    attribute_descriptions[3 + i].binding = 1;
	    attribute_descriptions[3 + i].location = 3 + i;
	    attribute_descriptions[3 + i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	    attribute_descriptions[3 + i].offset = sizeof(float) * 4 * i;
	}

	return attribute_descriptions;
    }   
};

#endif
