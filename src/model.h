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

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "alloc.h"

struct Model {
    struct Vertex {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texture;
    };
    
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    auto vertex_buffer_size() const noexcept -> std::size_t {
	return vertices.size() * sizeof(Vertex);
    }

    auto index_buffer_size() const noexcept -> std::size_t {
	return indices.size() * sizeof(uint32_t);
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
	memcpy(dst, indices.data(), indices.size() * sizeof(uint32_t));
    }
};

#endif
