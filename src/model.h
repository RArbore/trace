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

#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <array>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct Model {
    static std::array<VkVertexInputBindingDescription, 2> binding_descriptions() {
	std::array<VkVertexInputBindingDescription, 2> binding_descriptions {};
	binding_descriptions[0].binding = 0;
	binding_descriptions[0].stride = sizeof(glm::vec2);
	binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	binding_descriptions[1].binding = 1;
	binding_descriptions[1].stride = sizeof(glm::vec2);
	binding_descriptions[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return binding_descriptions;
    }

    static std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions() {
	std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions {};
	attribute_descriptions[0].binding = 0;
	attribute_descriptions[0].location = 0;
	attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attribute_descriptions[0].offset = 0;
	attribute_descriptions[1].binding = 1;
	attribute_descriptions[1].location = 1;
	attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribute_descriptions[1].offset = 0;

	return attribute_descriptions;
    }
    
    std::vector<glm::vec2> positions;
    std::vector<glm::vec3> colors;
};

static const Model simple_model {
    {
	{0.0f, -0.5f},
	{0.5f, 0.5f},
	{-0.5f, 0.5f}
    },
    {
	{1.0f, 0.0f, 0.0f},
	{0.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 1.0f}
    }
};

#endif
