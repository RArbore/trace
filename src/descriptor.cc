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

#include "context.h"

static constexpr uint32_t MAX_MODELS = 256;

auto RenderContext::create_sampler() noexcept -> void {
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
    vkDestroySampler(device, sampler, NULL);
}

auto RenderContext::create_descriptor_pool() noexcept -> void {
    VkDescriptorPoolSize descriptor_pool_sizes[] = {
	{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, FRAMES_IN_FLIGHT * MAX_MODELS },
	{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, FRAMES_IN_FLIGHT },
	{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, FRAMES_IN_FLIGHT * MAX_MODELS },
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
    descriptor_pool_create_info.poolSizeCount = sizeof(descriptor_pool_sizes) / sizeof(descriptor_pool_sizes[0]);
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes;
    descriptor_pool_create_info.maxSets = FRAMES_IN_FLIGHT * sizeof(descriptor_pool_sizes) / sizeof(descriptor_pool_sizes[0]);

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
    vkDestroyDescriptorPool(device, descriptor_pool, NULL);
    vkDestroyDescriptorPool(device, imgui_descriptor_pool, NULL);
}

auto RenderContext::create_descriptor_set_layout() noexcept -> void {
    VkDescriptorSetLayoutBinding lights_buffer_layout_binding {};
    lights_buffer_layout_binding.binding = 0;
    lights_buffer_layout_binding.descriptorCount = 1;
    lights_buffer_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lights_buffer_layout_binding.pImmutableSamplers = NULL;
    lights_buffer_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    
    VkDescriptorSetLayoutBinding bindless_textures_layout_binding {};
    bindless_textures_layout_binding.binding = 1;
    bindless_textures_layout_binding.descriptorCount = MAX_MODELS;
    bindless_textures_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindless_textures_layout_binding.pImmutableSamplers = NULL;
    bindless_textures_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    
    VkDescriptorSetLayoutBinding bindings[] = {lights_buffer_layout_binding, bindless_textures_layout_binding};
    
    VkDescriptorBindingFlags bindless_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    VkDescriptorBindingFlags bindings_flags[] = {0, bindless_flags};

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
    vkDestroyDescriptorSetLayout(device, raster_descriptor_set_layout, NULL);
}

auto RenderContext::create_ray_trace_descriptor_set_layout() noexcept -> void {
    VkDescriptorSetLayoutBinding tlas_layout_binding {};
    tlas_layout_binding.binding = 0;
    tlas_layout_binding.descriptorCount = 1;
    tlas_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    tlas_layout_binding.pImmutableSamplers = NULL;
    tlas_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding out_image_layout_binding {};
    out_image_layout_binding.binding = 1;
    out_image_layout_binding.descriptorCount = 1;
    out_image_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    out_image_layout_binding.pImmutableSamplers = NULL;
    out_image_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    
    VkDescriptorSetLayoutBinding bindings[] = {tlas_layout_binding, out_image_layout_binding};
    
    VkDescriptorSetLayoutCreateInfo layout_create_info {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    layout_create_info.pBindings = bindings;

    ASSERT(vkCreateDescriptorSetLayout(device, &layout_create_info, NULL, &ray_trace_descriptor_set_layout), "Unable to create descriptor set layout.");
}

auto RenderContext::cleanup_ray_trace_descriptor_set_layout() noexcept -> void {
    vkDestroyDescriptorSetLayout(device, ray_trace_descriptor_set_layout, NULL);
}

auto RenderContext::create_descriptor_sets() noexcept -> void {
    std::array<VkDescriptorSetLayout, FRAMES_IN_FLIGHT> layouts;
    std::array<uint32_t, FRAMES_IN_FLIGHT> max_variable_count;
    layouts.fill(raster_descriptor_set_layout);
    max_variable_count.fill(MAX_MODELS);
    
    VkDescriptorSetVariableDescriptorCountAllocateInfo variable_count_allocate_info {};
    variable_count_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    variable_count_allocate_info.descriptorSetCount = FRAMES_IN_FLIGHT;
    variable_count_allocate_info.pDescriptorCounts = max_variable_count.data();
    
    VkDescriptorSetAllocateInfo allocate_info {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = &variable_count_allocate_info;
    allocate_info.descriptorPool = descriptor_pool;
    allocate_info.descriptorSetCount = FRAMES_IN_FLIGHT;
    allocate_info.pSetLayouts = layouts.data();

    ASSERT(vkAllocateDescriptorSets(device, &allocate_info, raster_descriptor_sets.data()), "Unable to allocate descriptor sets.");
}

auto RenderContext::create_ray_trace_descriptor_sets() noexcept -> void {
    std::array<VkDescriptorSetLayout, FRAMES_IN_FLIGHT> layouts;
    std::array<uint32_t, FRAMES_IN_FLIGHT> max_variable_count;
    layouts.fill(ray_trace_descriptor_set_layout);
    max_variable_count.fill(MAX_MODELS);
    
    VkDescriptorSetAllocateInfo allocate_info {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = descriptor_pool;
    allocate_info.descriptorSetCount = FRAMES_IN_FLIGHT;
    allocate_info.pSetLayouts = layouts.data();

    ASSERT(vkAllocateDescriptorSets(device, &allocate_info, ray_trace_descriptor_sets.data()), "Unable to allocate descriptor sets.");
}

auto RenderContext::update_descriptors_textures(const RasterScene &scene, uint32_t update_texture) noexcept -> void {
    VkDescriptorImageInfo descriptor_image_info {};
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor_image_info.imageView = scene.textures[update_texture].second;
    descriptor_image_info.sampler = sampler;

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
	DescriptorWriteInfo entry {};
	std::get<2>(entry) = descriptor_image_info;
	auto it = raster_descriptor_set_writes.insert({current_frame + i, entry});

	VkWriteDescriptorSet &write_descriptor_set = std::get<0>(it->second);
	write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set.dstSet = raster_descriptor_sets[(current_frame + i) % FRAMES_IN_FLIGHT];
	write_descriptor_set.dstBinding = 1;
	write_descriptor_set.dstArrayElement = update_texture;
	write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_descriptor_set.descriptorCount = 1;
	write_descriptor_set.pImageInfo = &std::get<2>(it->second);
	write_descriptor_set.pBufferInfo = NULL;
	write_descriptor_set.pTexelBufferView = NULL;
	write_descriptor_set.pNext = NULL;
    }
}

auto RenderContext::update_descriptors_lights(const RasterScene &scene) noexcept -> void {
    VkDescriptorBufferInfo descriptor_buffer_info {};
    descriptor_buffer_info.buffer = scene.lights_buf.buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = VK_WHOLE_SIZE;

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
	DescriptorWriteInfo entry {};
	std::get<1>(entry) = descriptor_buffer_info;
	auto it = raster_descriptor_set_writes.insert({current_frame + i, entry});

	VkWriteDescriptorSet &write_descriptor_set = std::get<0>(it->second);
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set.dstSet = raster_descriptor_sets[(current_frame + i) % FRAMES_IN_FLIGHT];
	write_descriptor_set.dstBinding = 0;
	write_descriptor_set.dstArrayElement = 0;
	write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_set.descriptorCount = 1;
	write_descriptor_set.pImageInfo = NULL;
	write_descriptor_set.pBufferInfo = &std::get<1>(it->second);
	write_descriptor_set.pTexelBufferView = NULL;
	write_descriptor_set.pNext = NULL;
    }
}

auto RenderContext::update_descriptors_ray_trace(const RasterScene &scene) noexcept -> void {
    VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info {};
    descriptor_acceleration_structure_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    descriptor_acceleration_structure_info.accelerationStructureCount = 1;
    descriptor_acceleration_structure_info.pAccelerationStructures = &scene.tlas;

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
	VkDescriptorImageInfo descriptor_image_info {};
	descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	descriptor_image_info.imageView = ray_trace_image_views[i];

	{
	    DescriptorWriteInfo entry {};
	    std::get<3>(entry) = descriptor_acceleration_structure_info;
	    auto it = raster_descriptor_set_writes.insert({current_frame + i, entry});
	    
	    VkWriteDescriptorSet &write_descriptor_set = std::get<0>(it->second);
	    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	    write_descriptor_set.dstSet = ray_trace_descriptor_sets[(current_frame + i) % FRAMES_IN_FLIGHT];
	    write_descriptor_set.dstBinding = 0;
	    write_descriptor_set.dstArrayElement = 0;
	    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	    write_descriptor_set.descriptorCount = 1;
	    write_descriptor_set.pImageInfo = NULL;
	    write_descriptor_set.pBufferInfo = NULL;
	    write_descriptor_set.pTexelBufferView = NULL;
	    write_descriptor_set.pNext = &std::get<3>(it->second);
	}

	{
	    DescriptorWriteInfo entry {};
	    std::get<2>(entry) = descriptor_image_info;
	    auto it = raster_descriptor_set_writes.insert({current_frame + i, entry});
	    
	    VkWriteDescriptorSet &write_descriptor_set = std::get<0>(it->second);
	    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	    write_descriptor_set.dstSet = ray_trace_descriptor_sets[(current_frame + i) % FRAMES_IN_FLIGHT];
	    write_descriptor_set.dstBinding = 1;
	    write_descriptor_set.dstArrayElement = 0;
	    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	    write_descriptor_set.descriptorCount = 1;
	    write_descriptor_set.pImageInfo = &std::get<2>(it->second);
	    write_descriptor_set.pBufferInfo = NULL;
	    write_descriptor_set.pTexelBufferView = NULL;
	}
    }
}
