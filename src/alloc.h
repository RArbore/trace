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

#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <stb/stb_image.h>

struct Buffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    std::size_t size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memory_flags;
    VmaAllocationCreateFlags vma_flags;
};

struct Image {
    VkImage image;
    VmaAllocation allocation;
    VkExtent2D extent;
};

struct RingBuffer {
    struct RingElement {
	Buffer buffer;
	std::size_t occupied;
	VkCommandBuffer command_buffer;
	VkSemaphore semaphore;
    };

    static const std::size_t NOT_OCCUPIED = 0xFFFFFFFFFFFFFFFF;
    static const std::size_t MAX_ELEMENTS = 0xFFFF;

    std::unordered_map<VkBuffer, VkSemaphore> upload_buffer_semaphores;
    std::unordered_map<VkImage, VkSemaphore> upload_image_semaphores;
    std::vector<RingElement> elements;
    std::size_t last_copy_size;
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

struct AccelerationStructureBuilder {
    struct BuilderElement {
	Buffer scratch_buffer;
	std::size_t occupied;
	VkCommandBuffer command_buffer;
	VkSemaphore build_semaphore;
	VkSemaphore tlas_listen_build_semaphore;
	VkSemaphore tlas_instance_upload_semaphore;
    };

    static const std::size_t NOT_OCCUPIED = 0xFFFFFFFFFFFFFFFF;
    static const std::size_t MAX_ELEMENTS = 0xFFFF;

    std::unordered_map<VkAccelerationStructureKHR, VkSemaphore> build_acceleration_structure_semaphores;
    std::vector<BuilderElement> elements;

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
		*(dst++) = element.build_semaphore;
    }

    auto clear_occupied(std::size_t clear_frame) noexcept -> void {
	for (auto &element : elements)
	    if (element.occupied == clear_frame)
		element.occupied = RingBuffer::NOT_OCCUPIED;
    }
};

#endif
