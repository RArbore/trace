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

auto RenderContext::create_command_pool() noexcept -> void {
    const uint32_t queue_family = physical_check_queue_family(physical_device, (VkQueueFlagBits) (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));

    VkCommandPoolCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = queue_family;

    ASSERT(vkCreateCommandPool(device, &create_info, NULL, &command_pool), "Unable to create command pool.");
}

auto RenderContext::cleanup_command_pool() noexcept -> void {
    vkDestroyCommandPool(device, command_pool, NULL);
}

auto RenderContext::create_command_buffers() noexcept -> void {
    VkCommandBufferAllocateInfo allocate_info {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = FRAMES_IN_FLIGHT;

    ASSERT(vkAllocateCommandBuffers(device, &allocate_info, &raster_command_buffers[0]), "Unable to create command buffers.");
}

auto RenderContext::record_raster_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index, uint32_t flight_index, const Scene &scene) noexcept -> void {
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

    VkRenderPassBeginInfo render_pass_begin_info {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = raster_render_pass;
    render_pass_begin_info.framebuffer = swapchain_framebuffers[image_index];
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent = swapchain_extent;
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, raster_pipeline);

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapchain_extent.width;
    viewport.height = (float) swapchain_extent.width;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = swapchain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    const std::size_t offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &scene.vertices_buf.buffer, offsets);
    vkCmdBindVertexBuffers(command_buffer, 1, 1, &scene.instances_buf.buffer, offsets);
    vkCmdBindIndexBuffer(command_buffer, scene.indices_buf.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, raster_pipeline_layout, 0, 1, &raster_descriptor_sets[flight_index], 0, NULL);
    vkCmdPushConstants(command_buffer, raster_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &push_constants);
    vkCmdDrawIndexedIndirect(command_buffer, scene.indirect_draw_buf.buffer, 0, (uint32_t) scene.num_models, sizeof(VkDrawIndexedIndirectCommand));
    render_draw_data_wrapper_imgui(command_buffer);

    vkCmdEndRenderPass(command_buffer);

    ASSERT(vkEndCommandBuffer(command_buffer), "Something went wrong recording into a raster command buffer.");
}

auto RenderContext::record_ray_trace_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index, uint32_t flight_index) noexcept -> void {
    VkCommandBufferBeginInfo begin_info {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    ASSERT(vkBeginCommandBuffer(command_buffer, &begin_info), "Unable to begin recording command buffer.");

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, ray_trace_pipeline);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, ray_trace_pipeline_layout, 0, 1, &raster_descriptor_sets[flight_index], 0, NULL);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, ray_trace_pipeline_layout, 1, 1, &ray_trace_descriptor_sets[flight_index], 0, NULL);
    vkCmdPushConstants(command_buffer, ray_trace_pipeline_layout, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 0, sizeof(PushConstants), &push_constants);
    vkCmdTraceRaysKHR(command_buffer, &rgen_sbt_region, &miss_sbt_region, &hit_sbt_region, &call_sbt_region, swapchain_extent.width, swapchain_extent.height, 1);

    VkImageMemoryBarrier image_memory_barrier {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = swapchain_images[image_index];
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;
    image_memory_barrier.srcAccessMask = 0;
    image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);

    VkImageBlit blit_region {};
    blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.srcSubresource.mipLevel = 0;
    blit_region.srcSubresource.baseArrayLayer = 0;
    blit_region.srcSubresource.layerCount = 1;
    blit_region.srcOffsets[0].x = 0;
    blit_region.srcOffsets[0].y = 0;
    blit_region.srcOffsets[0].z = 0;
    blit_region.srcOffsets[1].x = swapchain_extent.width;
    blit_region.srcOffsets[1].y = swapchain_extent.height;
    blit_region.srcOffsets[1].z = 1;
    blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.dstSubresource.mipLevel = 0;
    blit_region.dstSubresource.baseArrayLayer = 0;
    blit_region.dstSubresource.layerCount = 1;
    blit_region.dstOffsets[0].x = 0;
    blit_region.dstOffsets[0].y = 0;
    blit_region.dstOffsets[0].z = 0;
    blit_region.dstOffsets[1].x = swapchain_extent.width;
    blit_region.dstOffsets[1].y = swapchain_extent.height;
    blit_region.dstOffsets[1].z = 1;
    vkCmdBlitImage(command_buffer, ray_trace_images[flight_index].image, VK_IMAGE_LAYOUT_GENERAL, swapchain_images[image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit_region, VK_FILTER_NEAREST);

    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
    
    ASSERT(vkEndCommandBuffer(command_buffer), "Something went wrong recording into a raster command buffer.");
}

auto RenderContext::create_sync_objects() noexcept -> void {
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
	image_available_semaphores[i] = create_semaphore();
	render_finished_semaphores[i] = create_semaphore();
	in_flight_fences[i] = create_fence();
    }
}

auto RenderContext::cleanup_sync_objects() noexcept -> void {
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
	vkDestroySemaphore(device, image_available_semaphores[i], NULL);
	vkDestroySemaphore(device, render_finished_semaphores[i], NULL);
	vkDestroyFence(device, in_flight_fences[i], NULL);
    }
}

auto RenderContext::create_semaphore() noexcept -> VkSemaphore {
    VkSemaphoreCreateInfo semaphore_info {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore result;
    ASSERT(vkCreateSemaphore(device, &semaphore_info, NULL, &result), "Unable to create semaphore.");
    return result;
}

auto RenderContext::create_fence() noexcept -> VkFence {
    VkFenceCreateInfo fence_info {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFence result;
    ASSERT(vkCreateFence(device, &fence_info, NULL, &result), "Unable to create fence.");
    return result;
}
