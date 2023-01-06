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
    allocate_info.commandBufferCount = 1;

    ASSERT(vkAllocateCommandBuffers(device, &allocate_info, &raster_command_buffer), "Unable to create command buffers.");
}
