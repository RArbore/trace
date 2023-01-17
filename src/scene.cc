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

#include <filesystem>

#include <tiny_obj_loader.h>

#include "context.h"

auto RenderContext::load_model(std::string_view model_name, RasterScene &scene) noexcept -> uint16_t {
    auto it = scene.loaded_models.find(std::string(model_name));
    if (it != scene.loaded_models.end())
	return it->second;
    
    const std::string obj_filepath =
	std::string("models/") +
	std::string(model_name) +
	std::string(".obj");
    std::string color_filepath =
	std::string("models/") +
	std::string(model_name) +
	std::string("PBRCOLOR.png");
    std::string normal_filepath =
	std::string("models/") +
	std::string(model_name) +
	std::string("PBRNORMAL.png");
    std::string rough_filepath =
	std::string("models/") +
	std::string(model_name) +
	std::string("PBRROUGH.png");
    std::string metal_filepath =
	std::string("models/") +
	std::string(model_name) +
	std::string("PBRMETAL.png");

    if (!std::filesystem::exists(color_filepath)) color_filepath = "models/DEFAULTPBRCOLOR.png";
    if (!std::filesystem::exists(normal_filepath)) normal_filepath = "models/DEFAULTPBRNORMAL.png";
    if (!std::filesystem::exists(rough_filepath)) rough_filepath = "models/DEFAULTPBRROUGH.png";
    if (!std::filesystem::exists(metal_filepath)) metal_filepath = "models/DEFAULTPBRMETAL.png";

    if (std::filesystem::exists(obj_filepath)) {
	const uint16_t model_id = scene.num_models;
	const uint16_t base_texture_id = scene.num_textures;

	scene.models.emplace_back(load_obj_model(obj_filepath));
	scene.textures.emplace_back(load_texture(color_filepath, true));
	scene.textures.emplace_back(load_texture(normal_filepath, false));
	scene.textures.emplace_back(load_texture(rough_filepath, false));
	scene.textures.emplace_back(load_texture(metal_filepath, false));
	scene.num_models += 1;
	scene.num_textures += 4;
	scene.transforms.emplace_back();
	scene.models.back().base_texture_id = base_texture_id;

	update_descriptors_textures(scene, base_texture_id);
	update_descriptors_textures(scene, base_texture_id + 1);
	update_descriptors_textures(scene, base_texture_id + 2);
	update_descriptors_textures(scene, base_texture_id + 3);

	scene.loaded_models.insert({std::string(model_name), model_id});

	std::cout << "INFO: Loaded model " << obj_filepath << ", with " << scene.models.back().vertices.size() << " vertices and " << scene.models.back().indices.size() << " indices.\n";
	std::cout << "INFO: Used PBR color texture at " << color_filepath << ".\n";
	std::cout << "INFO: Used PBR normal texture at " << normal_filepath << ".\n";
	std::cout << "INFO: Used PBR roughness texture at " << rough_filepath << ".\n";
	std::cout << "INFO: Used PBR metallicity texture at " << metal_filepath << ".\n";
	return model_id;
    } else {
	ASSERT(false, "Couldn't find model with given name. Currently, only .obj models are supported.");
	return {};
    }
}

auto RenderContext::load_obj_model(std::string_view obj_filepath) noexcept -> Model {
    Model model {};

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &obj_filepath[0]), "Unable to load OBJ model.");

    std::unordered_map<Model::Vertex, uint32_t> unique_vertices;

    for (const auto& shape : shapes) {
	for (const auto& index : shape.mesh.indices) {
	    Model::Vertex vertex {};

	    vertex.position = {
		attrib.vertices[3 * index.vertex_index + 0],
		attrib.vertices[3 * index.vertex_index + 1],
		attrib.vertices[3 * index.vertex_index + 2]
	    };
	    
	    if (index.normal_index >= 0) {
		vertex.normal = {
		    attrib.normals[3 * index.normal_index + 0],
		    attrib.normals[3 * index.normal_index + 1],
		    attrib.normals[3 * index.normal_index + 2]
		};
	    } else {
		ASSERT(false, "Model must contain vertex normals.");
	    }
	    
	    if (index.texcoord_index >= 0) {
		vertex.texture = {
		    attrib.texcoords[2 * index.texcoord_index + 0],
		    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
		};
	    } else {
		vertex.texture = {0.0f, 0.0f};
	    }
	    
	    model.vertices.push_back(vertex);
	    if (unique_vertices.count(vertex) == 0) {
		unique_vertices[vertex] = (uint32_t) model.vertices.size();
		model.vertices.push_back(vertex);
	    }
	    model.indices.push_back(unique_vertices[vertex]);
	}
    }
    
    return model;
}

auto RenderContext::load_texture(std::string_view texture_filepath, bool srgb) noexcept -> std::pair<Image, VkImageView> {
    int tex_width, tex_height, tex_channels;
    stbi_uc* pixels = stbi_load(&texture_filepath[0], &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    ASSERT(pixels, "Unable to load texture.");
    std::size_t image_size = tex_width * tex_height * 4;

    VkFormat format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    VkExtent2D extent = {(uint32_t) tex_width, (uint32_t) tex_height};
    Image dst = create_image(0, format, extent, 1, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    void *data_image = ringbuffer_claim_buffer(main_ring_buffer, image_size);
    memcpy(data_image, pixels, image_size);
    ringbuffer_submit_buffer(main_ring_buffer, dst);

    VkImageSubresourceRange subresource_range {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;

    return {dst, create_image_view(dst.image, format, subresource_range)};
}

auto RenderContext::build_acceleration_structure_for_scene(RasterScene &scene) noexcept -> void {
    auto &the_model = scene.models[0];
    auto &the_instance = scene.transforms[0][0];

    VkDeviceAddress vertex_buffer_address = get_device_address(scene.vertices_buf);
    VkDeviceAddress index_buffer_address = get_device_address(scene.indices_buf);

    VkAccelerationStructureGeometryTrianglesDataKHR geometry_triangles_data {};
    geometry_triangles_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry_triangles_data.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometry_triangles_data.vertexData.deviceAddress = vertex_buffer_address;
    geometry_triangles_data.vertexStride = sizeof(Model::Vertex);
    geometry_triangles_data.indexType = VK_INDEX_TYPE_UINT32;
    geometry_triangles_data.indexData.deviceAddress = index_buffer_address;
    //triangles.transformData = {};
    geometry_triangles_data.maxVertex = (uint32_t) the_model.vertices.size();
}
