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

#ifndef ALLOC_H
#define ALLOC_H

#include <vector>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct Buffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct Image {
    VkImage image;
    VmaAllocation allocation;
};

struct RingBuffer {
    struct RingElement {
	Buffer buffer;
	std::size_t size;
	std::size_t occupied;
	VkCommandBuffer command_buffer;
	VkSemaphore semaphore;
    };

    static const std::size_t NOT_OCCUPIED = 0xFFFFFFFFFFFFFFFF;

    std::vector<RingElement> elements;
    std::size_t last_size;
    uint16_t last_id;

    auto get_number_occupied(std::size_t current_frame) const noexcept -> uint16_t {
	uint16_t num = 0;
	for (auto &element : elements)
	    if (element.occupied == current_frame)
		++num;
	return num;
    }

    auto get_new_semaphores(VkSemaphore *dst, std::size_t current_frame) const noexcept -> void {
	for (auto &element : elements)
	    if (element.occupied == current_frame)
		*(dst++) = element.semaphore;
    }

    auto clear_occupied(std::size_t clear_frame) noexcept -> void {
	for (auto &element : elements)
	    if (element.occupied == clear_frame)
		element.occupied = RingBuffer::NOT_OCCUPIED;
    }
};

#endif
