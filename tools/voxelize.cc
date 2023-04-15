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
#include <math.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <tiny_obj_loader.h>

#include "Tracy.hpp"

#include "util.h"

#define NUM_THREADS 16

static int32_t resolution;

struct Triangle {
    glm::vec3 a, b, c;
};

struct Model {
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
};

auto usage() noexcept -> void {
    ZoneScoped;
    std::cout << "Usage: blue_noise_gen <obj model> <resolution>\n";
}

auto load_obj_model(std::string_view obj_filepath) noexcept -> Model {
    ZoneScoped;
    Model model {};
    glm::vec3 negative_corner, positive_corner;
    negative_corner.x = 100000.0f;
    negative_corner.y = 100000.0f;
    negative_corner.z = 100000.0f;
    positive_corner.x = -100000.0f;
    positive_corner.y = -100000.0f;
    positive_corner.z = -100000.0f;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &obj_filepath[0]), "Unable to load OBJ model.");

    std::unordered_map<glm::vec3, uint32_t> unique_vertices;

    uint32_t count = 0;
    for (const auto& shape : shapes) {
	for (const auto& index : shape.mesh.indices) {
	    glm::vec3 vertex;
	    vertex.x = attrib.vertices[3 * index.vertex_index + 0];
	    vertex.y = attrib.vertices[3 * index.vertex_index + 1];
	    vertex.z = attrib.vertices[3 * index.vertex_index + 2];
	    
	    if (unique_vertices.count(vertex) == 0) {
		unique_vertices[vertex] = (uint32_t) model.vertices.size();
		negative_corner.x = fmin(negative_corner.x, vertex.x);
		negative_corner.y = fmin(negative_corner.y, vertex.y);
		negative_corner.z = fmin(negative_corner.z, vertex.z);
		positive_corner.x = fmax(positive_corner.x, vertex.x);
		positive_corner.y = fmax(positive_corner.y, vertex.y);
		positive_corner.z = fmax(positive_corner.z, vertex.z);
		model.vertices.push_back(vertex);
	    }
	    model.indices.push_back(unique_vertices[vertex]);
	    ++count;
	}
    }

    float x_span = positive_corner.x - negative_corner.x;
    float y_span = positive_corner.y - negative_corner.y;
    float z_span = positive_corner.z - negative_corner.z;
    float max_span = fmax(fmax(x_span, y_span), z_span);
    glm::vec3 center;
    center.x = negative_corner.x + x_span / 2.0f;
    center.y = negative_corner.y + y_span / 2.0f;
    center.z = negative_corner.z + z_span / 2.0f;

    for (auto &v : model.vertices) {
	v.x = ((v.x - center.x) / max_span + 0.5f) * (float) resolution;
	v.y = ((v.y - center.y) / max_span + 0.5f) * (float) resolution;
	v.z = ((v.z - center.z) / max_span + 0.5f) * (float) resolution;
    }
    
    return model;
}

bool tri_aabb_sat(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 aabb, glm::vec3 axis) {
    float p0 = glm::dot(v0, axis);
    float p1 = glm::dot(v1, axis);
    float p2 = glm::dot(v2, axis);

    float r = aabb.x * glm::abs(glm::dot(glm::vec3(1, 0, 0), axis)) +
        aabb.y * glm::abs(glm::dot(glm::vec3(0, 1, 0), axis)) +
        aabb.z * glm::abs(glm::dot(glm::vec3(0, 0, 1), axis));

    float maxP = glm::max(p0, glm::max(p1, p2));
    float minP = glm::min(p0, glm::min(p1, p2));

    return !(glm::max(-maxP, minP) > r);
}

bool tri_aabb(Triangle &triangle, glm::vec3 aabb_center, glm::vec3 aabb_extents) {
    triangle.a -= aabb_center;
    triangle.b -= aabb_center;
    triangle.c -= aabb_center;

    glm::vec3 ab = glm::normalize(triangle.b - triangle.a);
    glm::vec3 bc = glm::normalize(triangle.c - triangle.b);
    glm::vec3 ca = glm::normalize(triangle.a - triangle.c);

    glm::vec3 a00 = glm::vec3(0.0, -ab.z, ab.y);
    glm::vec3 a01 = glm::vec3(0.0, -bc.z, bc.y);
    glm::vec3 a02 = glm::vec3(0.0, -ca.z, ca.y);

    glm::vec3 a10 = glm::vec3(ab.z, 0.0, -ab.x);
    glm::vec3 a11 = glm::vec3(bc.z, 0.0, -bc.x);
    glm::vec3 a12 = glm::vec3(ca.z, 0.0, -ca.x);

    glm::vec3 a20 = glm::vec3(-ab.y, ab.x, 0.0);
    glm::vec3 a21 = glm::vec3(-bc.y, bc.x, 0.0);
    glm::vec3 a22 = glm::vec3(-ca.y, ca.x, 0.0);

    if (
        !tri_aabb_sat(triangle.a, triangle.b, triangle.c, aabb_extents, a00) ||
        !tri_aabb_sat(triangle.a, triangle.b, triangle.c, aabb_extents, a01) ||
        !tri_aabb_sat(triangle.a, triangle.b, triangle.c, aabb_extents, a02) ||
        !tri_aabb_sat(triangle.a, triangle.b, triangle.c, aabb_extents, a10) ||
        !tri_aabb_sat(triangle.a, triangle.b, triangle.c, aabb_extents, a11) ||
        !tri_aabb_sat(triangle.a, triangle.b, triangle.c, aabb_extents, a12) ||
        !tri_aabb_sat(triangle.a, triangle.b, triangle.c, aabb_extents, a20) ||
        !tri_aabb_sat(triangle.a, triangle.b, triangle.c, aabb_extents, a21) ||
        !tri_aabb_sat(triangle.a, triangle.b, triangle.c, aabb_extents, a22) ||
        !tri_aabb_sat(triangle.a, triangle.b, triangle.c, aabb_extents, glm::vec3(1, 0, 0)) ||
        !tri_aabb_sat(triangle.a, triangle.b, triangle.c, aabb_extents, glm::vec3(0, 1, 0)) ||
        !tri_aabb_sat(triangle.a, triangle.b, triangle.c, aabb_extents, glm::vec3(0, 0, 1)) ||
        !tri_aabb_sat(triangle.a, triangle.b, triangle.c, aabb_extents, glm::cross(ab, bc))
    )
    {
        return false;
    }

    return true;
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

    Model model = load_obj_model(argv[1]);
    std::cout << "Voxelizing " << argv[1] << " at resolution of " << resolution << "^3 voxels.\n";

    std::vector<bool> voxel_grid(resolution * resolution * resolution);
    uint32_t num_filled = 0;
    for (uint32_t i = 0; i < model.indices.size() - 2; i += 3) {
	glm::vec3 tri[3];
	tri[0] = model.vertices[model.indices[i]];
	tri[1] = model.vertices[model.indices[i + 1]];
	tri[2] = model.vertices[model.indices[i + 2]];
	glm::vec3 negative_bound;
	negative_bound.x = fmin(fmin(tri[0].x, tri[1].x), tri[2].x);
	negative_bound.y = fmin(fmin(tri[0].y, tri[1].y), tri[2].y);
	negative_bound.z = fmin(fmin(tri[0].z, tri[1].z), tri[2].z);
	glm::vec3 positive_bound;
	positive_bound.x = fmax(fmax(tri[0].x, tri[1].x), tri[2].x);
	positive_bound.y = fmax(fmax(tri[0].y, tri[1].y), tri[2].y);
	positive_bound.z = fmax(fmax(tri[0].z, tri[1].z), tri[2].z);
	auto min = [](uint32_t x, uint32_t y) {
	    return x < y ? x : y;
	};
	auto max = [](uint32_t x, uint32_t y) {
	    return x > y ? x : y;
	};
	uint32_t negative_bound_fixed[3] = { min(max((uint32_t) negative_bound.x, 0), (uint32_t) resolution - 1), min(max((uint32_t) negative_bound.y, 0), (uint32_t) resolution - 1), min(max((uint32_t) negative_bound.z, 0), (uint32_t) resolution - 1)};
	uint32_t positive_bound_fixed[3] = { min(max((uint32_t) positive_bound.x, 0), (uint32_t) resolution - 1), min(max((uint32_t) positive_bound.y, 0), (uint32_t) resolution - 1), min(max((uint32_t) positive_bound.z, 0), (uint32_t) resolution - 1)};
	Triangle &triangle = *((Triangle *) &tri);
	for (uint32_t x = negative_bound_fixed[0]; x <= positive_bound_fixed[0]; ++x) {
	    for (uint32_t y = negative_bound_fixed[1]; y <= positive_bound_fixed[1]; ++y) {
		for (uint32_t z = negative_bound_fixed[2]; z <= positive_bound_fixed[2]; ++z) {
		    uint32_t voxel_grid_idx = x * resolution * resolution + y * resolution + z;
		    if (!voxel_grid[voxel_grid_idx] && tri_aabb(triangle, glm::vec3((float) x + 0.5f, (float) y + 0.5f, (float) z + 0.5f), glm::vec3(1.0f))) {
			++num_filled;
			voxel_grid[voxel_grid_idx] = true;
		    }
		}
	    }
	}
    }

    std::string output_file = std::string(argv[1]) + ".vox";
    FILE *f = fopen(output_file.c_str(), "w");
    ASSERT(f, "Couldn't open .vox file.");
    fwrite("VOX ", 1, 4, f);
    uint32_t version = 150;
    fwrite(&version, 1, 4, f);
    fwrite("MAIN", 1, 4, f);
    uint32_t main_size = 0;
    fwrite(&main_size, 1, 4, f);
    fwrite(&main_size, 1, 4, f); // TODO: Write proper main child chunks size
    fwrite("SIZE", 1, 4, f);
    fwrite(&main_size, 1, 4, f); // TODO: Write proper size chunk size
    fwrite(&main_size, 1, 4, f);
    fwrite(&resolution, 1, 4, f);
    fwrite(&resolution, 1, 4, f);
    fwrite(&resolution, 1, 4, f);
    fwrite("XYZI", 1, 4, f);
    uint32_t xyzi_size = num_filled * 4 + 4;
    fwrite(&xyzi_size, 1, 4, f);
    fwrite(&main_size, 1, 4, f); // TODO: Write proper xyzi child chunks size
    uint32_t num_voxels = num_filled;
    fwrite(&num_voxels, 1, 4, f);
    for (int32_t x = 0; x < resolution; ++x) {
	for (int32_t y = 0; y < resolution; ++y) {
	    for (int32_t z = 0; z < resolution; ++z) {
		uint32_t voxel_grid_idx = x * resolution * resolution + y * resolution + z;
		if (voxel_grid[voxel_grid_idx]) {
		    uint8_t voxel[4] = {(uint8_t) z, (uint8_t) y, (uint8_t) x, 1};
		    fwrite(voxel, 1, 4, f);
		}
	    }
	}
    }
    fwrite("RGBA", 1, 4, f);
    fwrite(&main_size, 1, 4, f); // TODO: Write proper rgba chunk size
    fwrite(&main_size, 1, 4, f); // TODO: Write proper rgba child chunks size
    for (uint32_t i = 0; i < 256; ++i) {
	uint32_t color = 0xFFFFFFFF;
	fwrite(&color, 1, 4, f);
    }
    
    fclose(f);
}
