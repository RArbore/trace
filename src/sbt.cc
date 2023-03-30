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

auto RenderContext::create_shader_binding_table() noexcept -> void {
    ZoneScoped;
    const uint32_t miss_count = 1;
    const uint32_t hit_count = 4;
    const uint32_t handle_count = 1 + miss_count + hit_count;

    auto align_up = [](uint32_t size, uint32_t alignment) {
	return (size + (alignment - 1)) & ~(alignment - 1);
    };

    const uint32_t handle_size_aligned = align_up(ray_tracing_properties.shaderGroupHandleSize, ray_tracing_properties.shaderGroupHandleAlignment);

    rgen_sbt_region.stride = align_up(handle_size_aligned, ray_tracing_properties.shaderGroupBaseAlignment);
    rgen_sbt_region.size = rgen_sbt_region.stride;
    miss_sbt_region.stride = handle_size_aligned;
    miss_sbt_region.size = align_up(miss_count * handle_size_aligned, ray_tracing_properties.shaderGroupBaseAlignment);
    hit_sbt_region.stride = handle_size_aligned;
    hit_sbt_region.size = align_up(hit_count * handle_size_aligned, ray_tracing_properties.shaderGroupBaseAlignment);

    uint32_t handles_size = handle_count * ray_tracing_properties.shaderGroupHandleSize;
    std::vector<char> handles(handles_size);
    ASSERT(vkGetRayTracingShaderGroupHandlesKHR(device, ray_trace_pipeline, 0, handle_count, handles_size, handles.data()), "Unable to fetch shader group handles from ray trace pipeline.");

    VkDeviceSize sbt_buffer_size = rgen_sbt_region.size + miss_sbt_region.size + hit_sbt_region.size;
    shader_binding_table_buffer = create_buffer(sbt_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, 0, "SHADER_BINDING_TABLE_BUFFER");
    VkDeviceAddress sbt_buffer_address = get_device_address(shader_binding_table_buffer);
    rgen_sbt_region.deviceAddress = sbt_buffer_address;
    miss_sbt_region.deviceAddress = sbt_buffer_address + rgen_sbt_region.size;
    hit_sbt_region.deviceAddress = sbt_buffer_address + rgen_sbt_region.size + miss_sbt_region.size;
    
    auto get_handle = [&](uint16_t i) { return handles.data() + i * ray_tracing_properties.shaderGroupHandleSize; };
    inefficient_upload_to_buffer([&](char *root_dst) {
	char *dst = root_dst;
	uint16_t handle_idx = 0;
	memcpy(dst, get_handle(handle_idx++), ray_tracing_properties.shaderGroupHandleSize);

	dst = root_dst + rgen_sbt_region.size;
	for (uint16_t i = 0; i < miss_count; ++i) {
	    memcpy(dst, get_handle(handle_idx++), ray_tracing_properties.shaderGroupHandleSize);
	    dst += miss_sbt_region.stride;
	}

	dst = root_dst + rgen_sbt_region.size + miss_sbt_region.size;
	for (uint16_t i = 0; i < hit_count; ++i) {
	    memcpy(dst, get_handle(handle_idx++), ray_tracing_properties.shaderGroupHandleSize);
	    dst += hit_sbt_region.stride;
	}
    }, sbt_buffer_size, shader_binding_table_buffer);
}

auto RenderContext::cleanup_shader_binding_table() noexcept -> void {
    ZoneScoped;
    cleanup_buffer(shader_binding_table_buffer);
}
