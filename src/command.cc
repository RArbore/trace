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

auto RenderContext::record_raster_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index, uint32_t flight_index, const RasterScene &scene) noexcept -> void {
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
    vkCmdPushConstants(command_buffer, raster_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4), &perspective_matrix[0][0]);
    vkCmdPushConstants(command_buffer, raster_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::mat4), &camera_matrix[0][0]);
    vkCmdDrawIndexedIndirect(command_buffer, scene.indirect_draw_buf.buffer, 0, (uint32_t) scene.num_models, sizeof(VkDrawIndexedIndirectCommand));
    render_draw_data_wrapper_imgui(command_buffer);

    vkCmdEndRenderPass(command_buffer);

    ASSERT(vkEndCommandBuffer(command_buffer), "Something went wrong recording into a raster command buffer.");
}

auto RenderContext::record_ray_trace_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index, uint32_t flight_index, const RasterScene &scene) noexcept -> void {
    VkCommandBufferBeginInfo begin_info {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    ASSERT(vkBeginCommandBuffer(command_buffer, &begin_info), "Unable to begin recording command buffer.");

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
