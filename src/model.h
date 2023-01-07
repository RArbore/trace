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
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

struct Buffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct Image {
    VkImage image;
    VmaAllocation allocation;
};

struct Model {
    std::vector<glm::vec2> positions;
    std::vector<glm::vec3> colors;
    std::vector<uint16_t> indices;

    Buffer vertices_buf, indices_buf;

    auto positions_buffer_size() const noexcept -> std::size_t {
	return positions.size() * sizeof(glm::vec2);
    }

    auto colors_buffer_size() const noexcept -> std::size_t {
	return colors.size() * sizeof(glm::vec3);
    }

    auto indices_buffer_size() const noexcept -> std::size_t {
	return indices.size() * sizeof(uint16_t);
    }

    auto total_buffer_size() const noexcept -> std::size_t {
	return positions.size() + colors.size() + indices.size();
    }

    auto offsets_into_buffer() const noexcept -> std::array<std::size_t, 2> {
	return {0, positions.size() * sizeof(glm::vec2)};
    }

    auto num_vertices() const noexcept -> uint32_t {
	return (uint32_t) positions.size();
    }

    auto num_indices() const noexcept -> uint32_t {
	return (uint32_t) indices.size();
    }

    static auto binding_descriptions() noexcept -> std::array<VkVertexInputBindingDescription, 2> {
	std::array<VkVertexInputBindingDescription, 2> binding_descriptions {};
	binding_descriptions[0].binding = 0;
	binding_descriptions[0].stride = sizeof(glm::vec2);
	binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	binding_descriptions[1].binding = 1;
	binding_descriptions[1].stride = sizeof(glm::vec3);
	binding_descriptions[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return binding_descriptions;
    }

    static auto attribute_descriptions() noexcept -> std::array<VkVertexInputAttributeDescription, 2> {
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
};

#endif
