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

auto RenderContext::create_command_pool() noexcept -> void {
    ZoneScoped;
    const uint32_t queue_family = physical_check_queue_family(physical_device, (VkQueueFlagBits) (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));

    VkCommandPoolCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = queue_family;

    ASSERT(vkCreateCommandPool(device, &create_info, NULL, &command_pool), "Unable to create command pool.");
}

auto RenderContext::cleanup_command_pool() noexcept -> void {
    ZoneScoped;
    vkDestroyCommandPool(device, command_pool, NULL);
}

auto RenderContext::create_command_buffers() noexcept -> void {
    ZoneScoped;
    VkCommandBufferAllocateInfo allocate_info {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    ASSERT(vkAllocateCommandBuffers(device, &allocate_info, &render_command_buffer), "Unable to create command buffers.");
}

auto RenderContext::record_render_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index, const Scene &scene) noexcept -> void {
    ZoneScoped;
    VkCommandBufferBeginInfo begin_info {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    ASSERT(vkBeginCommandBuffer(command_buffer, &begin_info), "Unable to begin recording command buffer.");

    VkClearValue clear_values[2];
    clear_values[0].color.float32[0] = 0.0f / 100.0f;
    clear_values[0].color.float32[1] = 0.0f / 100.0f;
    clear_values[0].color.float32[2] = 0.0f / 100.0f;
    clear_values[0].color.float32[3] = 1.0f;
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapchain_extent.width;
    viewport.height = (float) swapchain_extent.width;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = swapchain_extent;

    VkRenderPassBeginInfo render_pass_begin_info {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = motion_vector_render_pass;
    render_pass_begin_info.framebuffer = motion_vector_framebuffer;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent = swapchain_extent;
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, motion_vector_pipeline);
    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    const std::size_t offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &scene.vertices_buf.buffer, offsets);
    vkCmdBindVertexBuffers(command_buffer, 1, 1, &scene.instances_buf.buffer, offsets);
    vkCmdBindIndexBuffer(command_buffer, scene.indices_buf.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, raster_pipeline_layout, 0, 1, &raster_descriptor_set, 0, NULL);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, raster_pipeline_layout, 1, 1, &ray_trace_descriptor_set, 0, NULL);
    vkCmdPushConstants(command_buffer, raster_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &push_constants);
    vkCmdDrawIndexedIndirect(command_buffer, scene.indirect_draw_buf.buffer, 0, (uint32_t) scene.num_models, sizeof(VkDrawIndexedIndirectCommand));

    vkCmdEndRenderPass(command_buffer);

    push_constants.filter_iter = 0;
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, ray_trace_pipeline);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, ray_trace_pipeline_layout, 0, 1, &raster_descriptor_set, 0, NULL);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, ray_trace_pipeline_layout, 1, 1, &ray_trace_descriptor_set, 0, NULL);
    vkCmdPushConstants(command_buffer, ray_trace_pipeline_layout, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 0, sizeof(PushConstants), &push_constants);
    vkCmdTraceRaysKHR(command_buffer, &rgen_sbt_region, &miss_sbt_region, &hit_sbt_region, &call_sbt_region, swapchain_extent.width, swapchain_extent.height, 1);

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			 0, 0, NULL, 0, NULL, 0, NULL);

    if (imgui_data.temporal_filter) {
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, temporal_pipeline);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_layout, 0, 1, &raster_descriptor_set, 0, NULL);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_layout, 1, 1, &ray_trace_descriptor_set, 0, NULL);
	vkCmdPushConstants(command_buffer, compute_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &push_constants);
	vkCmdDispatch(command_buffer, (swapchain_extent.width + 31) / 32, (swapchain_extent.height + 31) / 32, 1);
	++push_constants.filter_iter;
    }

    if (imgui_data.atrous_filter_iters) {
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, atrous_pipeline);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_layout, 0, 1, &raster_descriptor_set, 0, NULL);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_layout, 1, 1, &ray_trace_descriptor_set, 0, NULL);
    } else {
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			     0, 0, NULL, 0, NULL, 0, NULL);
    }
    for (uint32_t filter_iter = 0; filter_iter < (uint32_t) imgui_data.atrous_filter_iters; ++filter_iter) {
	vkCmdPushConstants(command_buffer, compute_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &push_constants);
	vkCmdDispatch(command_buffer, (swapchain_extent.width + 31) / 32, (swapchain_extent.height + 31) / 32, 1);
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			     filter_iter + 1 < (uint32_t) imgui_data.atrous_filter_iters ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			     0, 0, NULL, 0, NULL, 0, NULL);
	++push_constants.filter_iter;
    }

    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.renderPass = raster_render_pass;
    render_pass_begin_info.framebuffer = swapchain_framebuffers[image_index];

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, raster_pipeline);
    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, raster_pipeline_layout, 0, 1, &raster_descriptor_set, 0, NULL);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, raster_pipeline_layout, 1, 1, &ray_trace_descriptor_set, 0, NULL);
    vkCmdPushConstants(command_buffer, raster_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &push_constants);
    vkCmdDraw(command_buffer, 6, 1, 0, 0);
    render_draw_data_wrapper_imgui(command_buffer);

    vkCmdEndRenderPass(command_buffer);

    ASSERT(vkEndCommandBuffer(command_buffer), "Something went wrong recording into a raster command buffer.");
}

auto RenderContext::create_sync_objects() noexcept -> void {
    ZoneScoped;
    image_available_semaphore = create_semaphore();
    render_finished_semaphore = create_semaphore();
    in_flight_fence = create_fence();
}

auto RenderContext::cleanup_sync_objects() noexcept -> void {
    ZoneScoped;
    vkDestroySemaphore(device, image_available_semaphore, NULL);
    vkDestroySemaphore(device, render_finished_semaphore, NULL);
    vkDestroyFence(device, in_flight_fence, NULL);
}

auto RenderContext::create_semaphore() noexcept -> VkSemaphore {
    ZoneScoped;
    VkSemaphoreCreateInfo semaphore_info {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore result;
    ASSERT(vkCreateSemaphore(device, &semaphore_info, NULL, &result), "Unable to create semaphore.");
    return result;
}

auto RenderContext::create_fence() noexcept -> VkFence {
    ZoneScoped;
    VkFenceCreateInfo fence_info {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFence result;
    ASSERT(vkCreateFence(device, &fence_info, NULL, &result), "Unable to create fence.");
    return result;
}
