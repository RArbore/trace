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
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "alloc.h"

struct Model {
    struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texture;

	bool operator==(const Vertex& other) const {
	    return position == other.position && normal == other.normal && texture == other.texture;
	}
    };
    
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint16_t base_texture_id;

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

    auto num_triangles() const noexcept -> uint32_t {
	return (uint32_t) indices.size() / 3;
    }

    auto dump_vertices(char *dst) const noexcept -> void {
	memcpy(dst, vertices.data(), vertices.size() * sizeof(Vertex));
    }

    auto dump_indices(char *dst) const noexcept -> void {
	memcpy(dst, indices.data(), indices.size() * sizeof(uint32_t));
    }

    auto get_texture_ids() const noexcept -> std::array<uint16_t, 4> {
	return {base_texture_id,
		(uint16_t) (base_texture_id + 1),
		(uint16_t) (base_texture_id + 2),
		(uint16_t) (base_texture_id + 3)};
    }
};

namespace std {
    template<> struct hash<Model::Vertex> {
        size_t operator()(Model::Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texture) << 1);
        }
    };
}

struct VoxelModel {
    std::vector<uint8_t> voxels;
    uint16_t x_len, y_len, z_len;
    std::array<uint32_t, 256> palette;
};

#endif
