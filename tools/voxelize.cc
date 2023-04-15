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

#include <unordered_map>
#include <iostream>
#include <vector>

#include <tiny_obj_loader.h>

#include "Tracy.hpp"

#include "util.h"

#define NUM_THREADS 16

static int32_t resolution;

struct Vertex {
    float x;
    float y;
    float z;

    bool operator==(const Vertex& other) const {
	return x == other.x && y == other.y && z == other.z;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<float>()(vertex.x) ^ (hash<float>()(vertex.y) << 1)) >> 1) ^ (hash<float>()(vertex.z) << 1);
        }
    };
}

struct Model {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

auto usage() noexcept -> void {
    ZoneScoped;
    std::cout << "Usage: blue_noise_gen <obj model> <resolution>\n";
}

auto load_obj_model(std::string_view obj_filepath) noexcept -> Model {
    ZoneScoped;
    Model model {};

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &obj_filepath[0]), "Unable to load OBJ model.");

    std::unordered_map<Vertex, uint32_t> unique_vertices;

    uint32_t count = 0;
    for (const auto& shape : shapes) {
	for (const auto& index : shape.mesh.indices) {
	    Vertex vertex;
	    vertex.x = attrib.vertices[3 * index.vertex_index + 0];
	    vertex.y = attrib.vertices[3 * index.vertex_index + 1];
	    vertex.z = attrib.vertices[3 * index.vertex_index + 2];
	    
	    model.vertices.push_back(vertex);
	    if (unique_vertices.count(vertex) == 0) {
		unique_vertices[vertex] = (uint32_t) model.vertices.size();
		model.vertices.push_back(vertex);
	    }
	    model.indices.push_back(unique_vertices[vertex]);
	    ++count;
	}
    }
    
    return model;
}

auto main(int32_t argc, char **argv) noexcept -> int32_t {
    ZoneScoped;
    FrameMark;
    if (argc != 3) {
	usage();
	exit(1);
    }

    int32_t num_chars;
    if (sscanf(argv[2], "%d%n", &resolution, &num_chars) != 1 || argv[2][num_chars] != '\0') {
	usage();
	exit(1);
    }
    ASSERT(resolution > 0 && (resolution & (resolution - 1)) == 0, "ERROR: Resolution must be a positive power of two.");

    Model obj_model = load_obj_model(argv[1]);
    std::cout << "Voxelizing " << argv[1] << " at resolution of " << resolution << "^3 voxels.\n";
}
