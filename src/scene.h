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

#include "model.h"

struct Scene {
    std::vector<Model> models;
    std::vector<glm::mat4> transforms;
    std::vector<uint16_t> model_ids;

    Buffer vertices_buf, indices_buf, instances_buf, indirect_draw_buf;
    std::vector<std::size_t> model_vertices_offsets, model_indices_offsets;

    auto num_objects() const noexcept -> std::size_t {
	return model_ids.size();
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

    static auto attribute_descriptions() noexcept -> std::array<VkVertexInputAttributeDescription, 6> {
	std::array<VkVertexInputAttributeDescription, 6> attribute_descriptions {};
	attribute_descriptions[0].binding = 0;
	attribute_descriptions[0].location = 0;
	attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribute_descriptions[0].offset = 0;
	attribute_descriptions[1].binding = 0;
	attribute_descriptions[1].location = 1;
	attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribute_descriptions[1].offset = sizeof(glm::vec3);

	for (uint32_t i = 0; i < 4; ++i) {
	    attribute_descriptions[2 + i].binding = 1;
	    attribute_descriptions[2 + i].location = 2 + i;
	    attribute_descriptions[2 + i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	    attribute_descriptions[2 + i].offset = sizeof(float) * 4 * i;
	}

	return attribute_descriptions;
    }   
};

#endif
