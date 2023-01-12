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

static constexpr uint32_t MAX_TEXTURES = 256;

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
    VkDescriptorPoolSize descriptor_pool_size {};
    descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_pool_size.descriptorCount = FRAMES_IN_FLIGHT * MAX_TEXTURES;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
    descriptor_pool_create_info.poolSizeCount = 1;
    descriptor_pool_create_info.pPoolSizes = &descriptor_pool_size;
    descriptor_pool_create_info.maxSets = FRAMES_IN_FLIGHT * MAX_TEXTURES;

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
    VkDescriptorSetLayoutBinding sampler_layout_binding {};
    sampler_layout_binding.binding = 0;
    sampler_layout_binding.descriptorCount = MAX_TEXTURES;
    sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.pImmutableSamplers = NULL;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutBinding bindings[] = {sampler_layout_binding};
    
    VkDescriptorBindingFlags bindless_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT layout_binding_flags_create_info {};
    layout_binding_flags_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    layout_binding_flags_create_info.bindingCount = 1;
    layout_binding_flags_create_info.pBindingFlags = &bindless_flags;

    VkDescriptorSetLayoutCreateInfo layout_create_info {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.pNext = &layout_binding_flags_create_info;
    layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
    layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    layout_create_info.pBindings = bindings;

    ASSERT(vkCreateDescriptorSetLayout(device, &layout_create_info, NULL, &raster_descriptor_set_layout), "Unable to create descriptor set layout.");
}

auto RenderContext::cleanup_descriptor_set_layout() noexcept -> void {
    vkDestroyDescriptorSetLayout(device, raster_descriptor_set_layout, NULL);
}

auto RenderContext::create_descriptor_sets() noexcept -> void {
    std::array<VkDescriptorSetLayout, FRAMES_IN_FLIGHT> layouts;
    std::array<uint32_t, FRAMES_IN_FLIGHT> max_variable_count;
    layouts.fill(raster_descriptor_set_layout);
    max_variable_count.fill(MAX_TEXTURES);
    
    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variable_count_allocate_info {};
    variable_count_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
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

auto RenderContext::update_descriptors(const RasterScene &scene, uint32_t update_texture) noexcept -> void {
    VkDescriptorImageInfo descriptor_image_info {};
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor_image_info.imageView = scene.textures[update_texture].second;
    descriptor_image_info.sampler = sampler;

    VkWriteDescriptorSet write_descriptor_set[FRAMES_IN_FLIGHT];
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
	write_descriptor_set[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set[i].dstSet = raster_descriptor_sets[i];
	write_descriptor_set[i].dstBinding = 0;
	write_descriptor_set[i].dstArrayElement = update_texture;
	write_descriptor_set[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_descriptor_set[i].descriptorCount = 1;
	write_descriptor_set[i].pImageInfo = &descriptor_image_info;
	write_descriptor_set[i].pBufferInfo = NULL;
	write_descriptor_set[i].pTexelBufferView = NULL;
	write_descriptor_set[i].pNext = NULL;
    }
    vkUpdateDescriptorSets(device, FRAMES_IN_FLIGHT, write_descriptor_set, 0, NULL);
}
