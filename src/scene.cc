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
#include <bit>

#include <tiny_obj_loader.h>

#include "Tracy.hpp"

#include "context.h"

auto RenderContext::allocate_vulkan_objects_for_scene(Scene &scene) noexcept -> void {
    ZoneScoped;
    scene.model_vertices_offsets.resize(scene.models.size());
    scene.model_indices_offsets.resize(scene.models.size());
    
    std::size_t vertex_idx = 0;
    const std::size_t vertex_size = std::accumulate(scene.models.begin(), scene.models.end(), 0, [&scene, &vertex_idx](const std::size_t &accum, const Model &model) { scene.model_vertices_offsets[vertex_idx++] = accum; return accum + model.vertex_buffer_size(); });
    scene.vertices_buf_contents_size = vertex_size;
    scene.vertices_buf = create_buffer(vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "SCENE_VERTICES_BUFFER");

    std::size_t index_idx = 0;
    const std::size_t index_size = std::accumulate(scene.models.begin(), scene.models.end(), 0, [&scene, &index_idx](const std::size_t &accum, const Model &model) { scene.model_indices_offsets[index_idx++] = accum; return accum + model.index_buffer_size(); });
    scene.indices_buf_contents_size = index_size;
    scene.indices_buf = create_buffer(index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "SCENE_INDICES_BUFFER");

    const std::size_t instance_size = scene.num_objects * sizeof(glm::mat4);
    scene.instances_buf_contents_size = instance_size;
    scene.instances_buf = create_buffer(instance_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "SCENE_INSTANCES_BUFFER");

    const std::size_t indirect_draw_size = scene.num_models * sizeof(VkDrawIndexedIndirectCommand);
    scene.indirect_draw_buf_contents_size = indirect_draw_size;
    scene.indirect_draw_buf = create_buffer(indirect_draw_size, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "SCENE_INDIRECT_DRAW_BUFFER");

    const std::size_t lights_size = (scene.num_lights + 1) * sizeof(glm::vec4);
    scene.lights_buf_contents_size = lights_size;
    scene.lights_buf = create_buffer(lights_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "SCENE_LIGHTS_BUFFER");

    const std::size_t ray_trace_objects_size = scene.num_objects * sizeof(Scene::RayTraceObject);
    scene.ray_trace_objects_buf_contents_size = ray_trace_objects_size;
    scene.ray_trace_objects_buf = create_buffer(ray_trace_objects_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "SCENE_RAY_TRACE_OBJECTS_BUFFER");

    ringbuffer_copy_scene_vertices_into_buffer(scene);
    ringbuffer_copy_scene_indices_into_buffer(scene);
    ringbuffer_copy_scene_instances_into_buffer(scene);
    ringbuffer_copy_scene_indirect_draw_into_buffer(scene);
    ringbuffer_copy_scene_lights_into_buffer(scene);
    ringbuffer_copy_scene_ray_trace_objects_into_buffer(scene);
}

auto RenderContext::update_vulkan_objects_for_scene(Scene &scene) noexcept -> void {
    ZoneScoped;
    scene.model_vertices_offsets.resize(scene.models.size());
    scene.model_indices_offsets.resize(scene.models.size());
    
    std::size_t vertex_idx = 0;
    const std::size_t vertex_size = std::accumulate(scene.models.begin(), scene.models.end(), 0, [&scene, &vertex_idx](const std::size_t &accum, const Model &model) { scene.model_vertices_offsets[vertex_idx++] = accum; return accum + model.vertex_buffer_size(); });
    scene.vertices_buf_contents_size = vertex_size;

    std::size_t index_idx = 0;
    const std::size_t index_size = std::accumulate(scene.models.begin(), scene.models.end(), 0, [&scene, &index_idx](const std::size_t &accum, const Model &model) { scene.model_indices_offsets[index_idx++] = accum; return accum + model.index_buffer_size(); });
    scene.indices_buf_contents_size = index_size;

    const std::size_t instance_size = scene.num_objects * sizeof(glm::mat4);
    scene.instances_buf_contents_size = instance_size;

    const std::size_t indirect_draw_size = scene.num_models * sizeof(VkDrawIndexedIndirectCommand);
    scene.indirect_draw_buf_contents_size = indirect_draw_size;

    const std::size_t lights_size = (scene.num_lights + 1) * sizeof(glm::vec4);
    scene.lights_buf_contents_size = lights_size;

    const std::size_t ray_trace_objects_size = scene.num_objects * sizeof(Scene::RayTraceObject);
    scene.ray_trace_objects_buf_contents_size = ray_trace_objects_size;

    ringbuffer_copy_scene_vertices_into_buffer(scene);
    ringbuffer_copy_scene_indices_into_buffer(scene);
    ringbuffer_copy_scene_instances_into_buffer(scene);
    ringbuffer_copy_scene_indirect_draw_into_buffer(scene);
    ringbuffer_copy_scene_lights_into_buffer(scene);
    ringbuffer_copy_scene_ray_trace_objects_into_buffer(scene);
}

auto RenderContext::cleanup_vulkan_objects_for_scene(Scene &scene) noexcept -> void {
    ZoneScoped;
    cleanup_buffer(scene.vertices_buf);
    cleanup_buffer(scene.indices_buf);
    cleanup_buffer(scene.instances_buf);
    cleanup_buffer(scene.indirect_draw_buf);
    cleanup_buffer(scene.lights_buf);
    cleanup_buffer(scene.ray_trace_objects_buf);
    for (auto image : scene.textures) {
	cleanup_image_view(image.second);
	cleanup_image(image.first);
    }
    for (auto volume : scene.voxel_volumes) {
	cleanup_image_view(volume.second);
	cleanup_volume(volume.first);
    }
    vkDestroyAccelerationStructureKHR(device, scene.tlas, NULL);
    cleanup_buffer(scene.tlas_buffer);
    cleanup_buffer(scene.tlas_instances_buffer);
    for (auto blas : scene.blass)
	vkDestroyAccelerationStructureKHR(device, blas, NULL);
    for (auto buffer : scene.blas_buffers)
	cleanup_buffer(buffer);
    for (auto blas : scene.voxel_blass)
	vkDestroyAccelerationStructureKHR(device, blas, NULL);
    for (auto buffer : scene.voxel_blas_buffers)
	cleanup_buffer(buffer);
}

auto RenderContext::ringbuffer_copy_scene_vertices_into_buffer(Scene &scene) noexcept -> void {
    ZoneScoped;
    char *data_vertex = (char *) ringbuffer_claim_buffer(main_ring_buffer, scene.vertices_buf_contents_size);
    for (const Model &model : scene.models) {
	model.dump_vertices(data_vertex);
	data_vertex += model.vertex_buffer_size();
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.vertices_buf);
}

auto RenderContext::ringbuffer_copy_scene_indices_into_buffer(Scene &scene) noexcept -> void {
    ZoneScoped;
    char *data_index = (char *) ringbuffer_claim_buffer(main_ring_buffer, scene.indices_buf_contents_size);
    for (const Model &model : scene.models) {
	model.dump_indices(data_index);
	data_index += model.index_buffer_size();
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.indices_buf);
}

auto RenderContext::ringbuffer_copy_scene_instances_into_buffer(Scene &scene) noexcept -> void {
    ZoneScoped;
    glm::mat4 *data_instance = (glm::mat4 *) ringbuffer_claim_buffer(main_ring_buffer, scene.instances_buf_contents_size);
    for (auto transforms : scene.transforms) {
	memcpy(data_instance, transforms.data(), transforms.size() * sizeof(glm::mat4));
	data_instance += transforms.size();
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.instances_buf);
}

auto RenderContext::ringbuffer_copy_scene_indirect_draw_into_buffer(Scene &scene) noexcept -> void {
    ZoneScoped;
    VkDrawIndexedIndirectCommand *data_indirect_draw  = (VkDrawIndexedIndirectCommand *) ringbuffer_claim_buffer(main_ring_buffer, scene.indirect_draw_buf_contents_size);
    uint32_t num_instances_so_far = 0;
    for (std::size_t i = 0; i < scene.num_models; ++i) {
	const std::size_t vertex_buffer_model_offset = scene.model_vertices_offsets[i];
	const std::size_t index_buffer_model_offset = scene.model_indices_offsets[i];
	data_indirect_draw[i].indexCount = scene.models[i].num_indices();
	data_indirect_draw[i].instanceCount = (uint32_t) scene.transforms[i].size();
	data_indirect_draw[i].firstIndex = (uint32_t) index_buffer_model_offset / sizeof(uint32_t);
	data_indirect_draw[i].vertexOffset = (int32_t) vertex_buffer_model_offset / sizeof(Model::Vertex);
	data_indirect_draw[i].firstInstance = num_instances_so_far;
	num_instances_so_far += (uint32_t) scene.transforms[i].size();
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.indirect_draw_buf);
}

auto RenderContext::ringbuffer_copy_scene_lights_into_buffer(Scene &scene) noexcept -> void {
    ZoneScoped;
    glm::vec4 *data_light = (glm::vec4 *) ringbuffer_claim_buffer(main_ring_buffer, scene.lights_buf_contents_size);
    (*data_light)[0] = std::bit_cast<float>((uint32_t) scene.num_lights);
    memcpy(data_light + 1, scene.lights.data(), scene.num_lights * sizeof(glm::vec4));
    ringbuffer_submit_buffer(main_ring_buffer, scene.lights_buf);
}

auto RenderContext::ringbuffer_copy_scene_ray_trace_objects_into_buffer(Scene &scene) noexcept -> void {
    ZoneScoped;
    Scene::RayTraceObject *data_ray_trace_object = (Scene::RayTraceObject *) ringbuffer_claim_buffer(main_ring_buffer, scene.ray_trace_objects_buf_contents_size);
    VkDeviceAddress vertex_buffer_address = get_device_address(scene.vertices_buf);
    VkDeviceAddress index_buffer_address = get_device_address(scene.indices_buf);
    for (std::size_t i = 0; i < scene.num_models; ++i) {
	for (std::size_t j = 0; j < scene.transforms[i].size(); ++j) {
	    data_ray_trace_object->vertex_address = vertex_buffer_address + scene.model_vertices_offsets[i];
	    data_ray_trace_object->index_address = index_buffer_address + scene.model_indices_offsets[i];
	    data_ray_trace_object->model_id = i;
	    ++data_ray_trace_object;
	}
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.ray_trace_objects_buf);
}

const glm::vec2 quincunx[5] = {
    glm::vec2(0.5, 0.5),
    glm::vec2(-0.5, -0.5),
    glm::vec2(0.0, 0.0),
    glm::vec2(-0.5, 0.5),
    glm::vec2(0.5, -0.5),
};

auto RenderContext::ringbuffer_copy_projection_matrices_into_buffer() noexcept -> void {
    ZoneScoped;
    glm::mat4 *data_mat = (glm::mat4 *) ringbuffer_claim_buffer(main_ring_buffer, PROJECTION_BUFFER_SIZE);
    data_mat[0] = glm::perspective(glm::radians(80.0f), 1.0f, 0.01f, 1000.0f);
    data_mat[0][1][1] *= -1.0f;
    data_mat[1] = glm::inverse(data_mat[0]);
    data_mat[2] = camera_matrix;
    data_mat[3] = last_frame_camera_matrix;
    data_mat[4] = glm::inverse(camera_matrix);
    data_mat[5] = glm::inverse(last_frame_camera_matrix);
    for (uint32_t i = 0; i < 4; ++i) {
	data_mat[i + 6] = data_mat[i + 2];
	data_mat[i + 6][3][0] = 0.0f;
	data_mat[i + 6][3][1] = 0.0f;
	data_mat[i + 6][3][2] = 0.0f;
    }
    data_mat[10] = data_mat[0];
    if (imgui_data.taa) {
	data_mat[10][3][0] += 2.0f * quincunx[current_frame % 5].x / (float) (swapchain_extent.width * swapchain_extent.width);
	data_mat[10][3][1] += 2.0f * quincunx[current_frame % 5].y / (float) (swapchain_extent.height * swapchain_extent.height);
    }
    data_mat[10] = glm::inverse(data_mat[10]);
    glm::vec4 *data_vec = (glm::vec4 *) &data_mat[11];
    *((glm::vec3 *) &data_vec[0]) = camera_position;
    *((glm::vec3 *) &data_vec[1]) = view_dir;
    *((glm::vec3 *) &data_vec[2]) = glm::normalize(glm::cross(view_dir, glm::vec3(0.0f, 0.0f, 1.0f)));
    *((glm::vec3 *) &data_vec[3]) = glm::cross(view_dir, *((glm::vec3 *) &data_vec[2]));
    *((glm::vec3 *) &data_vec[4]) = last_frame_camera_position;
    *((glm::vec3 *) &data_vec[5]) = last_frame_view_dir;
    *((glm::vec3 *) &data_vec[6]) = glm::normalize(glm::cross(last_frame_view_dir, glm::vec3(0.0f, 0.0f, 1.0f)));
    *((glm::vec3 *) &data_vec[7]) = glm::cross(last_frame_view_dir, *((glm::vec3 *) &data_vec[6]));
    ringbuffer_submit_buffer(main_ring_buffer, projection_buffer);
}

auto RenderContext::load_model(std::string_view model_name, Scene &scene, const uint8_t *custom_mat) noexcept -> uint16_t {
    ZoneScoped;
    auto it = scene.loaded_models.find(std::string(model_name));
    if (!custom_mat && it != scene.loaded_models.end())
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
	if (custom_mat) {
	    auto mat = load_custom_material(custom_mat[0], custom_mat[1], custom_mat[2], custom_mat[3], custom_mat[4], 0xD);
	    scene.textures.emplace_back(mat[0]);
	    scene.textures.emplace_back(load_texture(normal_filepath, false));
	    scene.textures.emplace_back(mat[2]);
	    scene.textures.emplace_back(mat[3]);
	} else {
	    scene.textures.emplace_back(load_texture(color_filepath, true));
	    scene.textures.emplace_back(load_texture(normal_filepath, false));
	    scene.textures.emplace_back(load_texture(rough_filepath, false));
	    scene.textures.emplace_back(load_texture(metal_filepath, false));
	}
	scene.num_models += 1;
	scene.num_textures += 4;
	scene.transforms.emplace_back();
	scene.blass.push_back(VK_NULL_HANDLE);
	scene.blas_buffers.emplace_back();
	scene.models.back().base_texture_id = base_texture_id;

	update_descriptors_textures(scene, base_texture_id);
	update_descriptors_textures(scene, base_texture_id + 1);
	update_descriptors_textures(scene, base_texture_id + 2);
	update_descriptors_textures(scene, base_texture_id + 3);
	
	if (!custom_mat)
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
    ZoneScoped;
    Model model {};

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &obj_filepath[0]), "Unable to load OBJ model.");

    std::unordered_map<Model::Vertex, uint32_t> unique_vertices;

    uint32_t count = 0;
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
		vertex.texture = {((float) count) / 1000.0f, ((float) count) / 1000.0f}; // Random texcoords for gradients in fragment shader
	    }
	    
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

auto RenderContext::load_texture(std::string_view texture_filepath, bool srgb) noexcept -> std::pair<Image, VkImageView> {
    ZoneScoped;
    int tex_width, tex_height, tex_channels;
    stbi_uc* pixels = stbi_load(&texture_filepath[0], &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    ASSERT(pixels, "Unable to load texture.");
    std::size_t image_size = tex_width * tex_height * 4;

    VkFormat format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    VkExtent2D extent = {(uint32_t) tex_width, (uint32_t) tex_height};
    Image dst = create_image(0, format, extent, 1, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "TEXTURE_IMAGE");

    void *data_image = ringbuffer_claim_buffer(main_ring_buffer, image_size);
    memcpy(data_image, pixels, image_size);
    ringbuffer_submit_buffer(main_ring_buffer, dst, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImageSubresourceRange subresource_range {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;

    return {dst, create_image_view(dst.image, format, subresource_range)};
}

auto RenderContext::load_image(std::string_view texture_filepath) noexcept -> std::pair<Image, VkImageView> {
    ZoneScoped;
    int tex_width, tex_height, tex_channels;
    stbi_uc* pixels = stbi_load(&texture_filepath[0], &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    ASSERT(pixels, "Unable to load texture.");
    std::size_t image_size = tex_width * tex_height * 4;

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    VkExtent2D extent = {(uint32_t) tex_width, (uint32_t) tex_height};
    Image dst = create_image(0, format, extent, 1, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "TEXTURE_IMAGE");

    void *data_image = ringbuffer_claim_buffer(main_ring_buffer, image_size);
    memcpy(data_image, pixels, image_size);
    ringbuffer_submit_buffer(main_ring_buffer, dst, VK_IMAGE_LAYOUT_GENERAL);

    VkImageSubresourceRange subresource_range {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;

    return {dst, create_image_view(dst.image, format, subresource_range)};
}

auto RenderContext::load_custom_model(const std::vector<Model::Vertex> &vertices, const std::vector<uint32_t> &indices, uint8_t red_albedo, uint8_t green_albedo, uint8_t blue_albedo, uint8_t roughness, uint8_t metallicity, Scene &scene) noexcept -> uint16_t {
    ZoneScoped;
    const uint16_t model_id = scene.num_models;
    const uint16_t base_texture_id = scene.num_textures;

    const auto mat = load_custom_material(red_albedo, green_albedo, blue_albedo, roughness, metallicity);
    scene.textures.emplace_back(mat[0]);
    scene.textures.emplace_back(mat[1]);
    scene.textures.emplace_back(mat[2]);
    scene.textures.emplace_back(mat[3]);
    update_descriptors_textures(scene, base_texture_id);
    update_descriptors_textures(scene, base_texture_id + 1);
    update_descriptors_textures(scene, base_texture_id + 2);
    update_descriptors_textures(scene, base_texture_id + 3);
    
    scene.models.emplace_back(vertices, indices, base_texture_id);

    scene.num_models += 1;
    scene.num_textures += 4;
    scene.transforms.emplace_back();
    scene.blass.push_back(VK_NULL_HANDLE);
    scene.blas_buffers.emplace_back();
    return model_id;
}

auto RenderContext::load_custom_material(uint8_t red_albedo, uint8_t green_albedo, uint8_t blue_albedo, uint8_t roughness, uint8_t metallicity, uint8_t mask) noexcept -> std::array<std::pair<Image, VkImageView>, 4> {
    ZoneScoped;
    std::size_t image_size = 4;
    std::array<uint8_t, 16> contents = {
	red_albedo, green_albedo, blue_albedo, 255,
	128, 128, 255, 255,
	roughness, roughness, roughness, 255,
	metallicity, metallicity, metallicity, 255
    };
    std::array<std::pair<Image, VkImageView>, 4> ret;

    for (uint16_t i = 0; i < 4; ++i) {
	if (!(mask & (1 << i)))
	    continue;
	
	VkFormat format = !i ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	VkExtent2D extent = {1, 1};
	Image dst = create_image(0, format, extent, 1, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "CUSTOM_MATERIAL_IMAGE");

	void *data_image = ringbuffer_claim_buffer(main_ring_buffer, image_size);
	memcpy(data_image, &contents[i * 4], image_size);
	ringbuffer_submit_buffer(main_ring_buffer, dst, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	
	VkImageSubresourceRange subresource_range {};
	subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel = 0;
	subresource_range.levelCount = 1;
	subresource_range.baseArrayLayer = 0;
	subresource_range.layerCount = 1;
	
	ret[i] = {dst, create_image_view(dst.image, format, subresource_range)};
    }

    return ret;
}

auto RenderContext::load_voxel_model(std::string_view model_name, Scene &scene) noexcept -> uint16_t {
    ZoneScoped;
    auto it = scene.loaded_voxel_models.find(std::string(model_name));
    if (it != scene.loaded_voxel_models.end())
	return it->second;
    const std::string vox_filepath =
	std::string("models/") +
	std::string(model_name) +
	std::string(".vox");

    if (std::filesystem::exists(vox_filepath)) {
	const uint16_t voxel_model_id = scene.num_voxel_models;
	scene.voxel_models.emplace_back(load_dot_vox_model(vox_filepath));
	scene.voxel_volumes.emplace_back(upload_voxel_model(scene.voxel_models.back()));
	
	++scene.num_voxel_models;
	scene.voxel_transforms.emplace_back();
	scene.voxel_blass.push_back(VK_NULL_HANDLE);
	scene.voxel_blas_buffers.emplace_back();

	update_descriptors_volumes(scene, voxel_model_id);

	std::cout << "INFO: Loaded voxel model " << vox_filepath << ".\n";
	return voxel_model_id;
    } else {
	ASSERT(false, "Couldn't find voxel model with given name. Currently, only .vox voxel models are supported.");
	return {};
    }
}

auto RenderContext::load_dot_vox_model(std::string_view vox_filepath) noexcept -> VoxelModel {
    ZoneScoped;
    VoxelModel model;
    auto file_size = std::filesystem::file_size(vox_filepath);
    std::vector<uint32_t> file_contents((file_size + sizeof(uint32_t) - 1) / sizeof(uint32_t));
    FILE *f = fopen(&vox_filepath[0], "r");
    ASSERT(f, "Couldn't open .vox file.");
    auto read_size = fread(file_contents.data(), 1, file_size, f);
    ASSERT(file_size == read_size, "Something went wrong reading .vox file.");

    auto convertID = [](const char id[4]) {
	return ((id[3] << 24) | (id[2] << 16) | (id[1] << 8) | id[0]);
    };
    ASSERT(file_contents[0] == convertID("VOX "), ".vox file contains incorrect magic number.");
    ASSERT(file_contents[2] == convertID("MAIN"), ".vox file doesn't contain MAIN chunk.");
    ASSERT(file_contents[3] == 0, ".vox file MAIN chunk is non-empty.");

    ASSERT(file_contents[5] == convertID("SIZE"), "First child chunk in .vox file is not a SIZE chunk (can only parse single model currently).");
    ASSERT(file_contents[7] == 0, "SIZE child chunk contains children.");
    model.x_len = (uint16_t) file_contents[8];
    model.y_len = (uint16_t) file_contents[9];
    model.z_len = (uint16_t) file_contents[10];

    ASSERT(file_contents[11] == convertID("XYZI"), "Second child chunk in .vox file is not a XYZI chunk (can only parse single model currently).");
    model.voxels.resize((uint32_t) model.x_len * (uint32_t) model.y_len * (uint32_t) model.z_len);
    uint32_t chunk_size = file_contents[12];
    ASSERT(file_contents[13] == 0, "XYZI child chunk contains children.");
    uint32_t next_chunk = 14 + chunk_size / (uint32_t) sizeof(uint32_t);
    uint32_t num_voxels = file_contents[14];
    for (uint32_t i = 0; i < num_voxels; ++i) {
	uint32_t voxel = file_contents[15 + i];
	uint8_t x = (uint8_t) (0xFF & (voxel));
	uint8_t y = (uint8_t) (0xFF & (voxel >> 8));
	uint8_t z = (uint8_t) (0xFF & (voxel >> 16));
	uint8_t v = (uint8_t) (0xFF & (voxel >> 24));
	model.voxels.at(x * model.y_len * model.z_len + y * model.z_len + z) = v;
    }

    return model;
}

auto RenderContext::upload_voxel_model(const VoxelModel &voxel_model) noexcept -> std::pair<Volume, VkImageView> {
    ZoneScoped;
    std::size_t volume_size = voxel_model.x_len * voxel_model.y_len * voxel_model.z_len;

    VkFormat format = VK_FORMAT_R8_UNORM;
    VkExtent3D extent =  {voxel_model.x_len, voxel_model.y_len, voxel_model.z_len};
    Volume dst = create_volume(0, format, extent, 1, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "VOLUME_IMAGE");

    void *data_image = ringbuffer_claim_buffer(main_ring_buffer, volume_size);
    memcpy(data_image, voxel_model.voxels.data(), volume_size);
    ringbuffer_submit_buffer(main_ring_buffer, dst, VK_IMAGE_LAYOUT_GENERAL);

    VkImageSubresourceRange subresource_range {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;

    return {dst, create_image3d_view(dst.image, format, subresource_range)};
}

auto glm4x4_to_vk_transform(const glm::mat4 &in, VkTransformMatrixKHR &out) noexcept -> void {
    ZoneScoped;
    for (uint16_t i = 0; i < 4; ++i) {
	out.matrix[0][i] = in[i][0];
	out.matrix[1][i] = in[i][1];
	out.matrix[2][i] = in[i][2];
    }
}

auto RenderContext::build_bottom_level_acceleration_structure_for_model(uint16_t model_idx, Scene &scene) noexcept -> void {
    ZoneScoped;
    const VkDeviceAddress vertex_buffer_address = get_device_address(scene.vertices_buf);
    const VkDeviceAddress index_buffer_address = get_device_address(scene.indices_buf);
    const VkDeviceSize alignment = acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment;

    VkAccelerationStructureGeometryTrianglesDataKHR geometry_triangles_data {};
    geometry_triangles_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry_triangles_data.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometry_triangles_data.vertexData.deviceAddress = vertex_buffer_address + scene.model_vertices_offsets[model_idx];
    geometry_triangles_data.vertexStride = sizeof(Model::Vertex);
    geometry_triangles_data.indexType = VK_INDEX_TYPE_UINT32;
    geometry_triangles_data.indexData.deviceAddress = index_buffer_address + scene.model_indices_offsets[model_idx];
    geometry_triangles_data.maxVertex = scene.models[model_idx].num_vertices();
    
    VkAccelerationStructureGeometryKHR blas_geometry {};
    blas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    blas_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    blas_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    blas_geometry.geometry.triangles = geometry_triangles_data;
    
    VkAccelerationStructureBuildRangeInfoKHR blas_build_range_info {};
    blas_build_range_info.firstVertex = 0;
    blas_build_range_info.primitiveCount = scene.models[model_idx].num_triangles();
    blas_build_range_info.primitiveOffset = 0;
    blas_build_range_info.transformOffset = 0;
    VkAccelerationStructureBuildRangeInfoKHR *blas_build_range_infos[] = {&blas_build_range_info};
    
    VkAccelerationStructureBuildGeometryInfoKHR blas_build_geometry_info {};
    blas_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    blas_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    blas_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    blas_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    blas_build_geometry_info.geometryCount = 1;
    blas_build_geometry_info.pGeometries = &blas_geometry;
    
    const uint32_t max_primitive_counts[] = {scene.models[model_idx].num_triangles()};
    
    VkAccelerationStructureBuildSizesInfoKHR blas_build_sizes_info {};
    blas_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &blas_build_geometry_info, max_primitive_counts, &blas_build_sizes_info);
    
    Buffer blas_build_scratch_buffer = create_buffer_with_alignment(blas_build_sizes_info.buildScratchSize, alignment, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "SCENE_BLAS_BUILD_SCRATCH_BUFFER");
    Buffer blas_acceleration_structure_buffer = create_buffer_with_alignment(blas_build_sizes_info.accelerationStructureSize, alignment, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "SCENE_BLAS_BUFFER");
    
    VkAccelerationStructureCreateInfoKHR bottom_level_create_info {};
    bottom_level_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    bottom_level_create_info.buffer = blas_acceleration_structure_buffer.buffer;
    bottom_level_create_info.offset = 0;
    bottom_level_create_info.size = blas_build_sizes_info.accelerationStructureSize;
    bottom_level_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    
    VkAccelerationStructureKHR bottom_level_acceleration_structure;
    ASSERT(vkCreateAccelerationStructureKHR(device, &bottom_level_create_info, NULL, &bottom_level_acceleration_structure), "Unable to create bottom level acceleration structure.");
    
    blas_build_geometry_info.dstAccelerationStructure = bottom_level_acceleration_structure;
    blas_build_geometry_info.scratchData.deviceAddress = get_device_address(blas_build_scratch_buffer);

    inefficient_run_commands([&](VkCommandBuffer cmd) {
	vkCmdBuildAccelerationStructuresKHR(cmd, 1, &blas_build_geometry_info, (const VkAccelerationStructureBuildRangeInfoKHR* const*) blas_build_range_infos);
    });

    cleanup_buffer(blas_build_scratch_buffer);
    scene.blass[model_idx] = bottom_level_acceleration_structure;
    scene.blas_buffers[model_idx] = blas_acceleration_structure_buffer;
}

auto RenderContext::build_bottom_level_acceleration_structure_for_voxel_model(uint16_t voxel_model_idx, Scene &scene) noexcept -> void {
    ZoneScoped;
    const VkDeviceSize alignment = acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment;

    VkAccelerationStructureGeometryAabbsDataKHR geometry_aabbs_data {};
    geometry_aabbs_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry_aabbs_data.data.deviceAddress = get_device_address(cube_buffer);
    geometry_aabbs_data.stride = sizeof(VkAabbPositionsKHR);
    
    VkAccelerationStructureGeometryKHR blas_geometry {};
    blas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    blas_geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
    blas_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR; // !!!
    blas_geometry.geometry.aabbs = geometry_aabbs_data;
    
    VkAccelerationStructureBuildRangeInfoKHR blas_build_range_info {};
    blas_build_range_info.firstVertex = 0;
    blas_build_range_info.primitiveCount = 1;
    blas_build_range_info.primitiveOffset = 0;
    blas_build_range_info.transformOffset = 0;
    VkAccelerationStructureBuildRangeInfoKHR *blas_build_range_infos[] = {&blas_build_range_info};
    
    VkAccelerationStructureBuildGeometryInfoKHR blas_build_geometry_info {};
    blas_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    blas_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    blas_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    blas_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    blas_build_geometry_info.geometryCount = 1;
    blas_build_geometry_info.pGeometries = &blas_geometry;
    
    const uint32_t max_primitive_counts[] = {1};
    
    VkAccelerationStructureBuildSizesInfoKHR blas_build_sizes_info {};
    blas_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &blas_build_geometry_info, max_primitive_counts, &blas_build_sizes_info);
    
    Buffer blas_build_scratch_buffer = create_buffer_with_alignment(blas_build_sizes_info.buildScratchSize, alignment, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "SCENE_BLAS_BUILD_SCRATCH_BUFFER");
    Buffer blas_acceleration_structure_buffer = create_buffer_with_alignment(blas_build_sizes_info.accelerationStructureSize, alignment, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "SCENE_BLAS_BUFFER");
    
    VkAccelerationStructureCreateInfoKHR bottom_level_create_info {};
    bottom_level_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    bottom_level_create_info.buffer = blas_acceleration_structure_buffer.buffer;
    bottom_level_create_info.offset = 0;
    bottom_level_create_info.size = blas_build_sizes_info.accelerationStructureSize;
    bottom_level_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    
    VkAccelerationStructureKHR bottom_level_acceleration_structure;
    ASSERT(vkCreateAccelerationStructureKHR(device, &bottom_level_create_info, NULL, &bottom_level_acceleration_structure), "Unable to create bottom level acceleration structure.");
    
    blas_build_geometry_info.dstAccelerationStructure = bottom_level_acceleration_structure;
    blas_build_geometry_info.scratchData.deviceAddress = get_device_address(blas_build_scratch_buffer);

    inefficient_run_commands([&](VkCommandBuffer cmd) {
	vkCmdBuildAccelerationStructuresKHR(cmd, 1, &blas_build_geometry_info, (const VkAccelerationStructureBuildRangeInfoKHR* const*) blas_build_range_infos);
    });

    cleanup_buffer(blas_build_scratch_buffer);
    scene.voxel_blass[voxel_model_idx] = bottom_level_acceleration_structure;
    scene.voxel_blas_buffers[voxel_model_idx] = blas_acceleration_structure_buffer;
}

auto RenderContext::build_top_level_acceleration_structure_for_scene(Scene &scene) noexcept -> void {
    ZoneScoped;
    const VkDeviceSize alignment = acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment;

    std::vector<VkAccelerationStructureInstanceKHR> bottom_level_instances;
    VkAccelerationStructureInstanceKHR bottom_level_instance {};
    bottom_level_instance.instanceCustomIndex = 0;
    for (uint16_t model_idx = 0; model_idx < scene.num_models; ++model_idx) {
	for (uint32_t transform_idx = 0; transform_idx < (uint32_t) scene.transforms[model_idx].size(); ++transform_idx) {
	    glm4x4_to_vk_transform(scene.transforms[model_idx][transform_idx], bottom_level_instance.transform);
	    bottom_level_instance.mask = 0xFF;
	    bottom_level_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	    bottom_level_instance.instanceShaderBindingTableRecordOffset = 0;
	    bottom_level_instance.accelerationStructureReference = get_device_address(scene.blass[model_idx]);
	    bottom_level_instances.push_back(bottom_level_instance);
	    ++bottom_level_instance.instanceCustomIndex;
	}
    }
    bottom_level_instance.instanceCustomIndex = 0;
    for (uint16_t voxel_model_idx = 0; voxel_model_idx < scene.num_voxel_models; ++voxel_model_idx) {
	for (uint32_t transform_idx = 0; transform_idx < (uint32_t) scene.voxel_transforms[voxel_model_idx].size(); ++transform_idx) {
	    glm4x4_to_vk_transform(scene.voxel_transforms[voxel_model_idx][transform_idx], bottom_level_instance.transform);
	    bottom_level_instance.mask = 0xFF;
	    bottom_level_instance.instanceShaderBindingTableRecordOffset = 1;
	    bottom_level_instance.accelerationStructureReference = get_device_address(scene.voxel_blass[voxel_model_idx]);
	    bottom_level_instances.push_back(bottom_level_instance);
	    ++bottom_level_instance.instanceCustomIndex;
	}
    }
    
    Buffer instances_buffer = create_buffer(bottom_level_instances.size() * sizeof(VkAccelerationStructureInstanceKHR), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, 0, "SCENE_TLAS_INSTANCES_BUFFER");
    inefficient_upload_to_buffer((void *) bottom_level_instances.data(), bottom_level_instances.size() * sizeof(VkAccelerationStructureInstanceKHR), instances_buffer);
    
    VkAccelerationStructureGeometryInstancesDataKHR geometry_instances_data {};
    geometry_instances_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry_instances_data.data.deviceAddress = get_device_address(instances_buffer);

    VkAccelerationStructureGeometryKHR tlas_geometry {};
    tlas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    tlas_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlas_geometry.geometry.instances = geometry_instances_data;
    
    VkAccelerationStructureBuildRangeInfoKHR tlas_build_range_info {};
    tlas_build_range_info.firstVertex = 0;
    tlas_build_range_info.primitiveCount = scene.num_objects + scene.num_voxel_objects;
    tlas_build_range_info.primitiveOffset = 0;
    tlas_build_range_info.transformOffset = 0;
    VkAccelerationStructureBuildRangeInfoKHR *tlas_build_range_infos[] = {&tlas_build_range_info}; 
    
    VkAccelerationStructureBuildGeometryInfoKHR tlas_build_geometry_info {};
    tlas_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    tlas_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlas_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    tlas_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    tlas_build_geometry_info.geometryCount = 1;
    tlas_build_geometry_info.pGeometries = &tlas_geometry;
    
    const uint32_t max_instances_counts[] = {scene.num_objects + scene.num_voxel_objects};
    
    VkAccelerationStructureBuildSizesInfoKHR tlas_build_sizes_info {};
    tlas_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &tlas_build_geometry_info, max_instances_counts, &tlas_build_sizes_info);
    
    Buffer tlas_build_scratch_buffer = create_buffer_with_alignment(tlas_build_sizes_info.buildScratchSize, alignment, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "SCENE_TLAS_BUILD_SCRATCH_BUFFER");
    Buffer tlas_acceleration_structure_buffer = create_buffer_with_alignment(tlas_build_sizes_info.accelerationStructureSize, alignment, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "SCENE_TLAS_BUFFER");
    
    VkAccelerationStructureCreateInfoKHR top_level_create_info {};
    top_level_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    top_level_create_info.buffer = tlas_acceleration_structure_buffer.buffer;
    top_level_create_info.offset = 0;
    top_level_create_info.size = tlas_build_sizes_info.accelerationStructureSize;
    top_level_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    VkAccelerationStructureKHR top_level_acceleration_structure;
    ASSERT(vkCreateAccelerationStructureKHR(device, &top_level_create_info, NULL, &top_level_acceleration_structure), "Unable to create top level acceleration structure.");

    tlas_build_geometry_info.dstAccelerationStructure = top_level_acceleration_structure;
    tlas_build_geometry_info.scratchData.deviceAddress = get_device_address(tlas_build_scratch_buffer);

    inefficient_run_commands([&](VkCommandBuffer cmd) {
	vkCmdBuildAccelerationStructuresKHR(cmd, 1, &tlas_build_geometry_info, (const VkAccelerationStructureBuildRangeInfoKHR* const*) tlas_build_range_infos);
    });

    scene.tlas = top_level_acceleration_structure;
    scene.tlas_buffer = tlas_acceleration_structure_buffer;
    scene.tlas_instances_buffer = instances_buffer;
    cleanup_buffer(tlas_build_scratch_buffer);
}
