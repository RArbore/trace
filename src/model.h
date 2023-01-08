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

#include <cstring>
#include <vector>
#include <array>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Buffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct Image {
    VkImage image;
    VmaAllocation allocation;
};

struct Model {
    struct Vertex {
	glm::vec3 position;
	glm::vec3 colors;
    };
    
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    auto vertex_buffer_size() const noexcept -> std::size_t {
	return vertices.size() * sizeof(Vertex);
    }

    auto index_buffer_size() const noexcept -> std::size_t {
	return indices.size() * sizeof(uint16_t);
    }

    auto num_vertices() const noexcept -> uint32_t {
	return (uint32_t) vertices.size();
    }

    auto num_indices() const noexcept -> uint32_t {
	return (uint32_t) indices.size();
    }

    auto dump_vertices(char *dst) const noexcept -> void {
	memcpy(dst, vertices.data(), vertices.size() * sizeof(Vertex));
    }

    auto dump_indices(char *dst) const noexcept -> void {
	memcpy(dst, indices.data(), indices.size() * sizeof(uint16_t));
    }
};

#endif
