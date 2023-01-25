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

auto RenderContext::allocate_vulkan_objects_for_scene(RasterScene &scene) noexcept -> void {
    scene.model_vertices_offsets.resize(scene.models.size());
    scene.model_indices_offsets.resize(scene.models.size());
    
    std::size_t vertex_idx = 0;
    const std::size_t vertex_size = std::accumulate(scene.models.begin(), scene.models.end(), 0, [&scene, &vertex_idx](const std::size_t &accum, const Model &model) { scene.model_vertices_offsets[vertex_idx++] = accum; return accum + model.vertex_buffer_size(); });
    scene.vertices_buf_contents_size = vertex_size;
    scene.vertices_buf = create_buffer(vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    std::size_t index_idx = 0;
    const std::size_t index_size = std::accumulate(scene.models.begin(), scene.models.end(), 0, [&scene, &index_idx](const std::size_t &accum, const Model &model) { scene.model_indices_offsets[index_idx++] = accum; return accum + model.index_buffer_size(); });
    scene.indices_buf_contents_size = index_size;
    scene.indices_buf = create_buffer(index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    const std::size_t instance_size = scene.num_objects * sizeof(glm::mat4);
    scene.instances_buf_contents_size = instance_size;
    scene.instances_buf = create_buffer(instance_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    const std::size_t indirect_draw_size = scene.num_models * sizeof(VkDrawIndexedIndirectCommand);
    scene.indirect_draw_buf_contents_size = indirect_draw_size;
    scene.indirect_draw_buf = create_buffer(indirect_draw_size, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    const std::size_t lights_size = scene.num_lights * sizeof(glm::vec4);
    scene.lights_buf_contents_size = lights_size;
    scene.lights_buf = create_buffer(lights_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    ringbuffer_copy_scene_vertices_into_buffer(scene);
    ringbuffer_copy_scene_indices_into_buffer(scene);
    ringbuffer_copy_scene_instances_into_buffer(scene);
    ringbuffer_copy_scene_indirect_draw_into_buffer(scene);
    ringbuffer_copy_scene_lights_into_buffer(scene);
}

auto RenderContext::update_vulkan_objects_for_scene(RasterScene &scene) noexcept -> void {
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

    const std::size_t lights_size = scene.num_lights * sizeof(glm::vec4);
    scene.lights_buf_contents_size = lights_size;

    ringbuffer_copy_scene_vertices_into_buffer(scene);
    ringbuffer_copy_scene_indices_into_buffer(scene);
    ringbuffer_copy_scene_instances_into_buffer(scene);
    ringbuffer_copy_scene_indirect_draw_into_buffer(scene);
    ringbuffer_copy_scene_lights_into_buffer(scene);
}

auto RenderContext::cleanup_vulkan_objects_for_scene(RasterScene &scene) noexcept -> void {
    cleanup_buffer(scene.vertices_buf);
    cleanup_buffer(scene.indices_buf);
    cleanup_buffer(scene.instances_buf);
    cleanup_buffer(scene.indirect_draw_buf);
    cleanup_buffer(scene.lights_buf);
    for (auto image : scene.textures) {
	cleanup_image_view(image.second);
	cleanup_image(image.first);
    }
}

auto RenderContext::ringbuffer_copy_scene_vertices_into_buffer(RasterScene &scene) noexcept -> void {
    char *data_vertex = (char *) ringbuffer_claim_buffer(main_ring_buffer, scene.vertices_buf_contents_size);
    for (const Model &model : scene.models) {
	model.dump_vertices(data_vertex);
	data_vertex += model.vertex_buffer_size();
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.vertices_buf);
}

auto RenderContext::ringbuffer_copy_scene_indices_into_buffer(RasterScene &scene) noexcept -> void {
    char *data_index = (char *) ringbuffer_claim_buffer(main_ring_buffer, scene.indices_buf_contents_size);
    for (const Model &model : scene.models) {
	model.dump_indices(data_index);
	data_index += model.index_buffer_size();
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.indices_buf);
}

auto RenderContext::ringbuffer_copy_scene_instances_into_buffer(RasterScene &scene) noexcept -> void {
    glm::mat4 *data_instance = (glm::mat4 *) ringbuffer_claim_buffer(main_ring_buffer, scene.instances_buf_contents_size);
    for (auto transforms : scene.transforms) {
	memcpy(data_instance, transforms.data(), transforms.size() * sizeof(glm::mat4));
	data_instance += transforms.size();
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.instances_buf);
}

auto RenderContext::ringbuffer_copy_scene_indirect_draw_into_buffer(RasterScene &scene) noexcept -> void {
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

auto RenderContext::ringbuffer_copy_scene_lights_into_buffer(RasterScene &scene) noexcept -> void {
    glm::vec4 *data_light = (glm::vec4 *) ringbuffer_claim_buffer(main_ring_buffer, scene.lights_buf_contents_size);
    memcpy(data_light, scene.lights.data(), scene.num_lights * sizeof(glm::vec4));
    ringbuffer_submit_buffer(main_ring_buffer, scene.lights_buf);
}

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

auto glm4x4_to_vk_transform(const glm::mat4 &in, VkTransformMatrixKHR &out) noexcept -> void {
    for (uint16_t i = 0; i < 4; ++i) {
	out.matrix[0][i] = in[i][0];
	out.matrix[1][i] = in[i][1];
	out.matrix[2][i] = in[i][2];
    }
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
    geometry_triangles_data.maxVertex = the_model.num_vertices();

    VkAccelerationStructureGeometryKHR blas_geometry {};
    blas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    blas_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    blas_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    blas_geometry.geometry.triangles = geometry_triangles_data;

    VkAccelerationStructureBuildRangeInfoKHR blas_build_range_info {};
    blas_build_range_info.firstVertex = 0;
    blas_build_range_info.primitiveCount = the_model.num_triangles();
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

    const uint32_t max_primitive_counts[] = {the_model.num_triangles()};

    VkAccelerationStructureBuildSizesInfoKHR blas_build_sizes_info {};
    blas_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &blas_build_geometry_info, max_primitive_counts, &blas_build_sizes_info);

    VkDeviceSize alignment = acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment;
    Buffer blas_build_scratch_buffer = create_buffer_with_alignment(blas_build_sizes_info.buildScratchSize, alignment, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    Buffer blas_acceleration_structure_buffer = create_buffer_with_alignment(blas_build_sizes_info.accelerationStructureSize, alignment, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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

    VkAccelerationStructureInstanceKHR bottom_level_instance {};
    glm4x4_to_vk_transform(the_instance, bottom_level_instance.transform);
    bottom_level_instance.instanceCustomIndex = 0;
    bottom_level_instance.mask = 0xFF;
    bottom_level_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    bottom_level_instance.instanceShaderBindingTableRecordOffset = 0;
    bottom_level_instance.accelerationStructureReference = get_device_address(bottom_level_acceleration_structure);

    Buffer instances_buffer = create_buffer(sizeof(VkAccelerationStructureInstanceKHR), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    inefficient_upload_to_buffer(&bottom_level_instance, sizeof(VkAccelerationStructureInstanceKHR), instances_buffer);

    VkAccelerationStructureGeometryInstancesDataKHR geometry_instances_data {};
    geometry_instances_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry_instances_data.data.deviceAddress = get_device_address(instances_buffer);
    VkAccelerationStructureGeometryKHR tlas_geometry {};
    tlas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    tlas_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlas_geometry.geometry.instances = geometry_instances_data;

    VkAccelerationStructureBuildRangeInfoKHR tlas_build_range_info {};
    tlas_build_range_info.firstVertex = 1;
    tlas_build_range_info.primitiveCount = 0;
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

    const uint32_t max_instances_counts[] = {1};
    
    VkAccelerationStructureBuildSizesInfoKHR tlas_build_sizes_info {};
    tlas_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &tlas_build_geometry_info, max_instances_counts, &tlas_build_sizes_info);

    Buffer tlas_build_scratch_buffer = create_buffer_with_alignment(tlas_build_sizes_info.buildScratchSize, alignment, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    Buffer tlas_acceleration_structure_buffer = create_buffer_with_alignment(tlas_build_sizes_info.accelerationStructureSize, alignment, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
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
}
