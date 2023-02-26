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

#include "Tracy.hpp"

#include "context.h"

static constexpr uint32_t MAX_MODELS = 256;

auto RenderContext::create_sampler() noexcept -> void {
    ZoneScoped;
    VkPhysicalDeviceProperties physical_device_properties {};
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

    VkSamplerCreateInfo sampler_create_info {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy = physical_device_properties.limits.maxSamplerAnisotropy;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 0.0f;

    ASSERT(vkCreateSampler(device, &sampler_create_info, NULL, &sampler), "Unable to create sampler.");
}

auto RenderContext::cleanup_sampler() noexcept -> void {
    ZoneScoped;
    vkDestroySampler(device, sampler, NULL);
}

auto RenderContext::create_descriptor_pool() noexcept -> void {
    ZoneScoped;
    VkDescriptorPoolSize descriptor_pool_sizes[] = {
	{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
	{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
	{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
	{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
	{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
	{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
	{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
	{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
	{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
	{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
	{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 },
	{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1000 },
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
    descriptor_pool_create_info.poolSizeCount = sizeof(descriptor_pool_sizes) / sizeof(descriptor_pool_sizes[0]);
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes;
    descriptor_pool_create_info.maxSets = sizeof(descriptor_pool_sizes) / sizeof(descriptor_pool_sizes[0]);

    ASSERT(vkCreateDescriptorPool(device, &descriptor_pool_create_info, NULL, &descriptor_pool), "Unable to create descriptor pool.");

    VkDescriptorPoolSize imgui_descriptor_pool_sizes[] = {
	{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
	{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
	{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
	{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
	{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
	{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
	{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
	{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
	{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
	{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
	{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo imgui_descriptor_pool_create_info {};
    imgui_descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    imgui_descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    imgui_descriptor_pool_create_info.maxSets = 1000;
    imgui_descriptor_pool_create_info.poolSizeCount = sizeof(imgui_descriptor_pool_sizes) / sizeof(imgui_descriptor_pool_sizes[0]);
    imgui_descriptor_pool_create_info.pPoolSizes = imgui_descriptor_pool_sizes;
    
    ASSERT(vkCreateDescriptorPool(device, &imgui_descriptor_pool_create_info, nullptr, &imgui_descriptor_pool), "Unable to create IMGUI descriptor pool.");
}

auto RenderContext::cleanup_descriptor_pool() noexcept -> void {
    ZoneScoped;
    vkDestroyDescriptorPool(device, descriptor_pool, NULL);
    vkDestroyDescriptorPool(device, imgui_descriptor_pool, NULL);
}

auto RenderContext::create_descriptor_set_layout() noexcept -> void {
    ZoneScoped;
    VkDescriptorSetLayoutBinding lights_buffer_layout_binding {};
    lights_buffer_layout_binding.binding = 0;
    lights_buffer_layout_binding.descriptorCount = 1;
    lights_buffer_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lights_buffer_layout_binding.pImmutableSamplers = NULL;
    lights_buffer_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding perspective_buffer_layout_binding {};
    perspective_buffer_layout_binding.binding = 1;
    perspective_buffer_layout_binding.descriptorCount = 1;
    perspective_buffer_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    perspective_buffer_layout_binding.pImmutableSamplers = NULL;
    perspective_buffer_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;
    
    VkDescriptorSetLayoutBinding bindless_textures_layout_binding {};
    bindless_textures_layout_binding.binding = 2;
    bindless_textures_layout_binding.descriptorCount = MAX_MODELS;
    bindless_textures_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindless_textures_layout_binding.pImmutableSamplers = NULL;
    bindless_textures_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;
    
    VkDescriptorSetLayoutBinding bindings[] = {lights_buffer_layout_binding, perspective_buffer_layout_binding, bindless_textures_layout_binding};
    
    VkDescriptorBindingFlags bindless_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    VkDescriptorBindingFlags bindings_flags[] = {0, 0, bindless_flags};

    VkDescriptorSetLayoutBindingFlagsCreateInfo layout_binding_flags_create_info {};
    layout_binding_flags_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    layout_binding_flags_create_info.bindingCount = sizeof(bindings_flags) / sizeof(bindings_flags[0]);
    layout_binding_flags_create_info.pBindingFlags = bindings_flags;

    VkDescriptorSetLayoutCreateInfo layout_create_info {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.pNext = &layout_binding_flags_create_info;
    layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    layout_create_info.pBindings = bindings;

    ASSERT(vkCreateDescriptorSetLayout(device, &layout_create_info, NULL, &raster_descriptor_set_layout), "Unable to create descriptor set layout.");
}

auto RenderContext::cleanup_descriptor_set_layout() noexcept -> void {
    ZoneScoped;
    vkDestroyDescriptorSetLayout(device, raster_descriptor_set_layout, NULL);
}

auto RenderContext::create_ray_trace_descriptor_set_layout() noexcept -> void {
    ZoneScoped;
    VkDescriptorSetLayoutBinding tlas_layout_binding {};
    tlas_layout_binding.binding = 0;
    tlas_layout_binding.descriptorCount = 1;
    tlas_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    tlas_layout_binding.pImmutableSamplers = NULL;
    tlas_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding ray_trace_objects_layout_binding {};
    ray_trace_objects_layout_binding.binding = 1;
    ray_trace_objects_layout_binding.descriptorCount = 1;
    ray_trace_objects_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ray_trace_objects_layout_binding.pImmutableSamplers = NULL;
    ray_trace_objects_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding blue_noise_image_layout_binding {};
    blue_noise_image_layout_binding.binding = 2;
    blue_noise_image_layout_binding.descriptorCount = 1;
    blue_noise_image_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    blue_noise_image_layout_binding.pImmutableSamplers = NULL;
    blue_noise_image_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding ray_trace_image_layout_bindings[12];
    for (uint32_t i = 0; i < 12; ++i) {
	ray_trace_image_layout_bindings[i].binding = 3 + i;
	ray_trace_image_layout_bindings[i].descriptorCount = 1;
	ray_trace_image_layout_bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	ray_trace_image_layout_bindings[i].pImmutableSamplers = NULL;
	ray_trace_image_layout_bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutBinding ray_trace_texture_layout_bindings[12];
    for (uint32_t i = 0; i < 12; ++i) {
	ray_trace_texture_layout_bindings[i].binding = 15 + i;
	ray_trace_texture_layout_bindings[i].descriptorCount = 1;
	ray_trace_texture_layout_bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	ray_trace_texture_layout_bindings[i].pImmutableSamplers = NULL;
	ray_trace_texture_layout_bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutBinding motion_vector_texture_layout_binding {};
    motion_vector_texture_layout_binding.binding = 27;
    motion_vector_texture_layout_binding.descriptorCount = 1;
    motion_vector_texture_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    motion_vector_texture_layout_binding.pImmutableSamplers = NULL;
    motion_vector_texture_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding taa_image_layout_bindings[2];
    for (uint32_t i = 0; i < 2; ++i) {
	taa_image_layout_bindings[i].binding = 28 + i;
	taa_image_layout_bindings[i].descriptorCount = 1;
	taa_image_layout_bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	taa_image_layout_bindings[i].pImmutableSamplers = NULL;
	taa_image_layout_bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;
    }
    
    VkDescriptorSetLayoutBinding taa_texture_layout_bindings[2];
    for (uint32_t i = 0; i < 2; ++i) {
	taa_texture_layout_bindings[i].binding = 30 + i;
	taa_texture_layout_bindings[i].descriptorCount = 1;
	taa_texture_layout_bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	taa_texture_layout_bindings[i].pImmutableSamplers = NULL;
	taa_texture_layout_bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT;
    }
    
    VkDescriptorSetLayoutBinding bindings[] = {
	tlas_layout_binding,
	ray_trace_objects_layout_binding,
	blue_noise_image_layout_binding,
	ray_trace_image_layout_bindings[0],
	ray_trace_image_layout_bindings[1],
	ray_trace_image_layout_bindings[2],
	ray_trace_image_layout_bindings[3],
	ray_trace_image_layout_bindings[4],
	ray_trace_image_layout_bindings[5],
	ray_trace_image_layout_bindings[6],
	ray_trace_image_layout_bindings[7],
	ray_trace_image_layout_bindings[8],
	ray_trace_image_layout_bindings[9],
	ray_trace_image_layout_bindings[10],
	ray_trace_image_layout_bindings[11],
	ray_trace_texture_layout_bindings[0],
	ray_trace_texture_layout_bindings[1],
	ray_trace_texture_layout_bindings[2],
	ray_trace_texture_layout_bindings[3],
	ray_trace_texture_layout_bindings[4],
	ray_trace_texture_layout_bindings[5],
	ray_trace_texture_layout_bindings[6],
	ray_trace_texture_layout_bindings[7],
	ray_trace_texture_layout_bindings[8],
	ray_trace_texture_layout_bindings[9],
	ray_trace_texture_layout_bindings[10],
	ray_trace_texture_layout_bindings[11],
	motion_vector_texture_layout_binding,
	taa_image_layout_bindings[0],
	taa_image_layout_bindings[1],
	taa_texture_layout_bindings[0],
	taa_texture_layout_bindings[1],
    };
    
    VkDescriptorSetLayoutCreateInfo layout_create_info {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    layout_create_info.pBindings = bindings;

    ASSERT(vkCreateDescriptorSetLayout(device, &layout_create_info, NULL, &ray_trace_descriptor_set_layout), "Unable to create descriptor set layout.");
}

auto RenderContext::cleanup_ray_trace_descriptor_set_layout() noexcept -> void {
    ZoneScoped;
    vkDestroyDescriptorSetLayout(device, ray_trace_descriptor_set_layout, NULL);
}

auto RenderContext::create_descriptor_sets() noexcept -> void {
    ZoneScoped;
    uint32_t max_variable_count = MAX_MODELS;
    
    VkDescriptorSetVariableDescriptorCountAllocateInfo variable_count_allocate_info {};
    variable_count_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    variable_count_allocate_info.descriptorSetCount = 1;
    variable_count_allocate_info.pDescriptorCounts = &max_variable_count;
    
    VkDescriptorSetAllocateInfo allocate_info {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = &variable_count_allocate_info;
    allocate_info.descriptorPool = descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &raster_descriptor_set_layout;
    
    ASSERT(vkAllocateDescriptorSets(device, &allocate_info, &raster_descriptor_set), "Unable to allocate descriptor sets.");
}

auto RenderContext::create_ray_trace_descriptor_sets() noexcept -> void {
    ZoneScoped;
    VkDescriptorSetAllocateInfo allocate_info {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &ray_trace_descriptor_set_layout;
    
    ASSERT(vkAllocateDescriptorSets(device, &allocate_info, &ray_trace_descriptor_set), "Unable to allocate descriptor sets.");
}

auto RenderContext::update_descriptors_textures(const Scene &scene, uint32_t update_texture) noexcept -> void {
    ZoneScoped;
    VkDescriptorImageInfo descriptor_image_info {};
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor_image_info.imageView = scene.textures[update_texture].second;
    descriptor_image_info.sampler = sampler;
    
    VkWriteDescriptorSet write_descriptor_set {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = raster_descriptor_set;
    write_descriptor_set.dstBinding = 2;
    write_descriptor_set.dstArrayElement = update_texture;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = NULL;
    write_descriptor_set.pTexelBufferView = NULL;
    write_descriptor_set.pNext = NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
}

auto RenderContext::update_descriptors_lights(const Scene &scene) noexcept -> void {
    ZoneScoped;
    VkDescriptorBufferInfo descriptor_buffer_info {};
    descriptor_buffer_info.buffer = scene.lights_buf.buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet write_descriptor_set {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = raster_descriptor_set;
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pImageInfo = NULL;
    write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
    write_descriptor_set.pTexelBufferView = NULL;
    write_descriptor_set.pNext = NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
}

auto RenderContext::update_descriptors_perspective() noexcept -> void {
    ZoneScoped;
    VkDescriptorBufferInfo descriptor_buffer_info {};
    descriptor_buffer_info.buffer = projection_buffer.buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet write_descriptor_set {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = raster_descriptor_set;
    write_descriptor_set.dstBinding = 1;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pImageInfo = NULL;
    write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
    write_descriptor_set.pTexelBufferView = NULL;
    write_descriptor_set.pNext = NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
}

auto RenderContext::update_descriptors_tlas(const Scene &scene) noexcept -> void {
    ZoneScoped;
    VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info {};
    descriptor_acceleration_structure_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    descriptor_acceleration_structure_info.accelerationStructureCount = 1;
    descriptor_acceleration_structure_info.pAccelerationStructures = &scene.tlas;
    
    VkWriteDescriptorSet write_descriptor_set {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = ray_trace_descriptor_set;
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pImageInfo = NULL;
    write_descriptor_set.pBufferInfo = NULL;
    write_descriptor_set.pTexelBufferView = NULL;
    write_descriptor_set.pNext = &descriptor_acceleration_structure_info;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
}

auto RenderContext::update_descriptors_ray_trace_objects(const Scene &scene) noexcept -> void {
    ZoneScoped;
    VkDescriptorBufferInfo descriptor_buffer_info {};
    descriptor_buffer_info.buffer = scene.ray_trace_objects_buf.buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet write_descriptor_set {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = ray_trace_descriptor_set;
    write_descriptor_set.dstBinding = 1;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pImageInfo = NULL;
    write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
    write_descriptor_set.pTexelBufferView = NULL;
    write_descriptor_set.pNext = NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
}

auto RenderContext::update_descriptors_blue_noise_images() noexcept -> void {
    ZoneScoped;
    VkDescriptorImageInfo descriptor_image_info {};
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    descriptor_image_info.imageView = blue_noise_image_view;
    
    VkWriteDescriptorSet write_descriptor_set {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = ray_trace_descriptor_set;
    write_descriptor_set.dstBinding = 2;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = NULL;
    write_descriptor_set.pTexelBufferView = NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
}

auto RenderContext::update_descriptors_ray_trace_images() noexcept -> void {
    ZoneScoped;
    VkDescriptorImageInfo descriptor_image_info {};
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet write_descriptor_set {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = ray_trace_descriptor_set;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = NULL;
    write_descriptor_set.pTexelBufferView = NULL;

    for (uint32_t i = 0; i < ray_trace1_image_views.size(); ++i) {
	write_descriptor_set.dstBinding = 3 + i;
	descriptor_image_info.imageView = ray_trace1_image_views[i];
	vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
    }
    
    for (uint32_t i = 0; i < ray_trace2_image_views.size(); ++i) {
	write_descriptor_set.dstBinding = 9 + i;
	descriptor_image_info.imageView = ray_trace2_image_views[i];
	vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
    }

    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_image_info.sampler = sampler;

    for (uint32_t i = 0; i < ray_trace1_image_views.size(); ++i) {
	write_descriptor_set.dstBinding = 15 + i;
	descriptor_image_info.imageView = ray_trace1_image_views[i];
	vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
    }
    
    for (uint32_t i = 0; i < ray_trace2_image_views.size(); ++i) {
	write_descriptor_set.dstBinding = 21 + i;
	descriptor_image_info.imageView = ray_trace2_image_views[i];
	vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
    }
}

auto RenderContext::update_descriptors_motion_vector_texture() noexcept -> void {
    ZoneScoped;
    VkDescriptorImageInfo descriptor_image_info {};
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor_image_info.imageView = motion_vector_image_view;
    descriptor_image_info.sampler = sampler;
    
    VkWriteDescriptorSet write_descriptor_set {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = ray_trace_descriptor_set;
    write_descriptor_set.dstBinding = 27;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = NULL;
    write_descriptor_set.pTexelBufferView = NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
}

auto RenderContext::update_descriptors_taa_images() noexcept -> void {
    ZoneScoped;
    VkDescriptorImageInfo descriptor_image_info {};
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet write_descriptor_set {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = ray_trace_descriptor_set;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = NULL;
    write_descriptor_set.pTexelBufferView = NULL;

    for (uint32_t i = 0; i < 2; ++i) {
	write_descriptor_set.dstBinding = 28 + i;
	descriptor_image_info.imageView = taa_image_views[i];
	vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
    }
    
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_image_info.sampler = sampler;

    for (uint32_t i = 0; i < 2; ++i) {
	write_descriptor_set.dstBinding = 30 + i;
	descriptor_image_info.imageView = taa_image_views[i];
	vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
    }
}
