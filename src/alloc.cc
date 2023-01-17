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

auto RenderContext::create_allocator() noexcept -> void {
    VmaAllocatorCreateInfo create_info {};
    create_info.instance = instance;
    create_info.physicalDevice = physical_device;
    create_info.device = device;
    create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    ASSERT(vmaCreateAllocator(&create_info, &allocator), "Couldn't create allocator.");
}

auto RenderContext::cleanup_allocator() noexcept -> void {
    vmaDestroyAllocator(allocator);
}

auto RenderContext::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_flags, VmaAllocationCreateFlags vma_flags) noexcept -> Buffer {
    VkBufferCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info {};
    alloc_info.flags = vma_flags;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.requiredFlags = memory_flags;

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    if (size > 0)
	ASSERT(vmaCreateBuffer(allocator, &create_info, &alloc_info, &buffer, &allocation, nullptr), "Unable to create buffer.");
    return {buffer, allocation, size, usage, memory_flags, vma_flags};
}

auto RenderContext::cleanup_buffer(Buffer buffer) noexcept -> void {
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

auto RenderContext::future_cleanup_buffer(Buffer buffer) noexcept -> void {
    buffer_cleanup_queue.push_back({buffer, current_frame});
}

auto RenderContext::create_image(VkImageCreateFlags flags, VkFormat format, VkExtent2D extent, uint32_t mip_levels, uint32_t array_layers, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_flags, VmaAllocationCreateFlags vma_flags) noexcept -> Image {
    VkImageCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.flags = flags;
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.format = format;
    create_info.extent.width = extent.width;
    create_info.extent.height = extent.height;
    create_info.extent.depth = 1;
    create_info.mipLevels = mip_levels;
    create_info.arrayLayers = array_layers;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo alloc_info {};
    alloc_info.flags = vma_flags;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.requiredFlags = memory_flags;

    VkImage image;
    VmaAllocation allocation;
    
    ASSERT(vmaCreateImage(allocator, &create_info, &alloc_info, &image, &allocation, nullptr), "Unable to create image.");
    return {image, allocation, extent};
}

auto RenderContext::create_image_view(VkImage image, VkFormat format, VkImageSubresourceRange subresource_range) noexcept -> VkImageView {
    VkImageViewCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = format;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.subresourceRange = subresource_range;

    VkImageView view;
    ASSERT(vkCreateImageView(device, &create_info, NULL, &view), "Unable to create image view.");
    
    return view;
}

auto RenderContext::cleanup_image(Image image) noexcept -> void {
    vmaDestroyImage(allocator, image.image, image.allocation);
}

auto RenderContext::cleanup_image_view(VkImageView view) noexcept -> void {
    vkDestroyImageView(device, view, NULL);
}

auto RenderContext::create_depth_resources() noexcept -> void {
    depth_image = create_image(0, VK_FORMAT_D32_SFLOAT, swapchain_extent, 1, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkImageSubresourceRange subresource_range {}; 
    subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;
    depth_image_view = create_image_view(depth_image.image, VK_FORMAT_D32_SFLOAT, subresource_range);
}

auto RenderContext::cleanup_depth_resources() noexcept -> void {
    cleanup_image_view(depth_image_view);
    cleanup_image(depth_image);
}

auto RenderContext::create_ringbuffer() noexcept -> RingBuffer {
    return {};
}

auto RenderContext::cleanup_ringbuffer(RingBuffer &ring_buffer) noexcept -> void {
    for (auto &element : ring_buffer.elements) {
	cleanup_buffer(element.buffer);
	vkDestroySemaphore(device, element.semaphore, NULL);
    }
    ring_buffer.elements.clear();
    for (auto [_, semaphore] : ring_buffer.upload_buffer_semaphores) {
	vkDestroySemaphore(device, semaphore, NULL);
    }
    ring_buffer.upload_buffer_semaphores.clear();
    for (auto [_, semaphore] : ring_buffer.upload_image_semaphores) {
	vkDestroySemaphore(device, semaphore, NULL);
    }
    ring_buffer.upload_image_semaphores.clear();
}

static inline auto round_up_p2(std::size_t v) noexcept -> std::size_t {
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    ++v;
    return v;
}

auto RenderContext::ringbuffer_claim_buffer(RingBuffer &ring_buffer, std::size_t size) noexcept -> void * {
    ring_buffer.last_copy_size = size;
    if (size == 0)
	return NULL;
    for (ring_buffer.last_id = 0; ring_buffer.last_id < ring_buffer.elements.size(); ++ring_buffer.last_id) {
	auto &element = ring_buffer.elements[ring_buffer.last_id];
	if (element.size >= size && element.occupied == RingBuffer::NOT_OCCUPIED) {
	    break;
	}
    }
    if (ring_buffer.last_id == ring_buffer.elements.size()) {
	ASSERT(ring_buffer.elements.size() < RingBuffer::MAX_ELEMENTS, "Too many elements in ring buffer.");
	std::size_t new_element_size = round_up_p2(size);
	Buffer new_element_buffer = create_buffer(new_element_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
	VkSemaphore new_element_semaphore = create_semaphore();

	VkCommandBufferAllocateInfo allocate_info {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool = command_pool;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = 1;

	VkCommandBuffer new_element_command_buffer;
	ASSERT(vkAllocateCommandBuffers(device, &allocate_info, &new_element_command_buffer), "Unable to create command buffers.");

	ring_buffer.elements.push_back({
		new_element_buffer,
		new_element_size,
		RingBuffer::NOT_OCCUPIED,
		new_element_command_buffer,
		new_element_semaphore
	    });
    }

    ring_buffer.elements[ring_buffer.last_id].occupied = current_frame;

    void *mapped_memory;
    vmaMapMemory(allocator, ring_buffer.elements[ring_buffer.last_id].buffer.allocation, &mapped_memory);
    return mapped_memory;
}

auto RenderContext::ringbuffer_submit_buffer(RingBuffer &ring_buffer, Buffer &dst) noexcept -> void {
    if (ring_buffer.last_copy_size == 0)
	return;
    vmaUnmapMemory(allocator, ring_buffer.elements[ring_buffer.last_id].buffer.allocation);

    if (ring_buffer.last_copy_size > dst.size) {
	future_cleanup_buffer(dst);
	dst = create_buffer(ring_buffer.last_copy_size * 2, dst.usage, dst.memory_flags, dst.vma_flags);
    }

    VkBufferCopy copy_region {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = ring_buffer.last_copy_size;

    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkCommandBuffer command_buffer = ring_buffer.elements[ring_buffer.last_id].command_buffer;
    
    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    
    vkCmdCopyBuffer(command_buffer, ring_buffer.elements[ring_buffer.last_id].buffer.buffer, dst.buffer, 1, &copy_region);
    
    vkEndCommandBuffer(command_buffer);
    
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    VkSemaphore signal_semaphore = ring_buffer.elements[ring_buffer.last_id].semaphore;
    VkSubmitInfo submit_info {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    auto it = ring_buffer.upload_buffer_semaphores.find(dst.buffer);
    VkSemaphore prev_semaphore;
    if (it != ring_buffer.upload_buffer_semaphores.end()) {
	prev_semaphore = it->second;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &prev_semaphore;
	submit_info.pWaitDstStageMask = wait_stages;
    } else {
	prev_semaphore = create_semaphore();
	ring_buffer.upload_buffer_semaphores[dst.buffer] = prev_semaphore;
    }
    VkSemaphore signal_semaphores[] = {prev_semaphore, signal_semaphore};
    submit_info.signalSemaphoreCount = 2;
    submit_info.pSignalSemaphores = signal_semaphores;
    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
}

auto RenderContext::ringbuffer_submit_buffer(RingBuffer &ring_buffer, Image dst) noexcept -> void {
    vmaUnmapMemory(allocator, ring_buffer.elements[ring_buffer.last_id].buffer.allocation);

    VkImageMemoryBarrier image_memory_barrier {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = dst.image;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;
    image_memory_barrier.srcAccessMask = 0;
    image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    VkBufferImageCopy copy_region {};
    copy_region.bufferOffset = 0;
    copy_region.bufferRowLength = 0;
    copy_region.bufferImageHeight = 0;
    copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.imageSubresource.layerCount = 1;
    copy_region.imageOffset = {0, 0, 0};
    copy_region.imageExtent = {dst.extent.width, dst.extent.height, 1};

    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkCommandBuffer command_buffer = ring_buffer.elements[ring_buffer.last_id].command_buffer;
    
    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
    vkCmdCopyBufferToImage(command_buffer, ring_buffer.elements[ring_buffer.last_id].buffer.buffer, dst.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
    
    vkEndCommandBuffer(command_buffer);
    
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    VkSemaphore signal_semaphore = ring_buffer.elements[ring_buffer.last_id].semaphore;
    VkSubmitInfo submit_info {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    auto it = ring_buffer.upload_image_semaphores.find(dst.image);
    VkSemaphore prev_semaphore;
    if (it != ring_buffer.upload_image_semaphores.end()) {
	prev_semaphore = it->second;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &prev_semaphore;
	submit_info.pWaitDstStageMask = wait_stages;
    } else {
	prev_semaphore = create_semaphore();
	ring_buffer.upload_image_semaphores[dst.image] = prev_semaphore;
    }
    VkSemaphore signal_semaphores[] = {prev_semaphore, signal_semaphore};
    submit_info.signalSemaphoreCount = 2;
    submit_info.pSignalSemaphores = signal_semaphores;
    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
}

auto RenderContext::get_device_address(const Buffer &buffer) noexcept -> VkDeviceAddress {
    VkBufferDeviceAddressInfo buffer_device_address_info {};
    buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    buffer_device_address_info.buffer = buffer.buffer;
    return vkGetBufferDeviceAddress(device, &buffer_device_address_info);
}
