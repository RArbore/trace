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
#include <map>

#include "model.h"
#include "util.h"

struct RasterScene {
    static const uint32_t MAX_LIGHTS = 512;
    
    std::vector<Model> models;
    std::vector<std::vector<glm::mat4>> transforms;
    std::vector<std::pair<Image, VkImageView>> textures;
    std::vector<glm::vec4> lights;
    uint16_t num_models;
    uint32_t num_objects;
    uint16_t num_textures;
    uint16_t num_lights;

    Buffer vertices_buf, indices_buf, instances_buf, indirect_draw_buf, lights_buf;
    std::size_t vertices_buf_contents_size, indices_buf_contents_size, instances_buf_contents_size, indirect_draw_buf_contents_size, lights_buf_contents_size;
    std::vector<std::size_t> model_vertices_offsets, model_indices_offsets;
    std::map<std::string, uint16_t> loaded_models;
    std::map<std::string, uint16_t> loaded_textures;

    auto add_model(auto &model_thunk, const char *obj_filepath) noexcept -> uint16_t {
	auto it = loaded_models.find(std::string(obj_filepath));
	if (it == loaded_models.end()) {
	    models.push_back(model_thunk());
	    transforms.push_back({});
	    loaded_models.insert({std::string(obj_filepath), num_models});
	    ++num_models;
	    return num_models - 1;
	} else {
	    return it->second;
	}
    }

    auto add_object(const glm::mat4 &&transform, uint16_t model_id) noexcept -> void {
	transforms.at(model_id).emplace_back(transform);
	++num_objects;
    }

    auto add_texture(auto &image_thunk, const char *texture_filepath) noexcept -> uint16_t {
	auto it = loaded_textures.find(std::string(texture_filepath));
	if (it == loaded_textures.end()) {
	    textures.push_back(image_thunk());
	    loaded_textures.insert({std::string(texture_filepath), num_textures});
	    ++num_textures;
	    return num_textures - 1;
	} else {
	    return it->second;
	}
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
