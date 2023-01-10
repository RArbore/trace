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

#define VMA_IMPLEMENTATION

#include "context.h"

auto RenderContext::create_allocator() noexcept -> void {
    VmaAllocatorCreateInfo create_info {};
    create_info.instance = instance;
    create_info.physicalDevice = physical_device;
    create_info.device = device;

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

    VkBuffer buffer;
    VmaAllocation allocation;

    ASSERT(vmaCreateBuffer(allocator, &create_info, &alloc_info, &buffer, &allocation, nullptr), "Unable to create buffer.");
    return {buffer, allocation};
}

auto RenderContext::cleanup_buffer(Buffer buffer) noexcept -> void {
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

auto RenderContext::create_image(VkImageCreateFlags flags, VkFormat format, VkExtent2D extent, uint32_t mipLevels, uint32_t arrayLevels, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_flags, VmaAllocationCreateFlags vma_flags) noexcept -> Image {
    VkImageCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.flags = flags;
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.format = format;
    create_info.extent.width = extent.width;
    create_info.extent.height = extent.height;
    create_info.extent.depth = 1;
    create_info.mipLevels = mipLevels;
    create_info.arrayLayers = arrayLevels;
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
    return {image, allocation};
}

auto RenderContext::create_image_view(VkImage image, VkFormat format, VkImageSubresourceRange subresource_range) noexcept -> VkImageView {
    VkImageViewCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = format;
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

auto RenderContext::allocate_vulkan_objects_for_scene(RasterScene &scene) noexcept -> void {
    scene.model_vertices_offsets.resize(scene.models.size());
    scene.model_indices_offsets.resize(scene.models.size());
    
    std::size_t vertex_idx = 0;
    const std::size_t vertex_size = std::accumulate(scene.models.begin(), scene.models.end(), 0, [&scene, &vertex_idx](const std::size_t &accum, const Model &model) { scene.model_vertices_offsets[vertex_idx++] = accum; return accum + model.vertex_buffer_size(); });
    scene.vertices_buf = create_buffer(vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    std::size_t index_idx = 0;
    const std::size_t index_size = std::accumulate(scene.models.begin(), scene.models.end(), 0, [&scene, &index_idx](const std::size_t &accum, const Model &model) { scene.model_indices_offsets[index_idx++] = accum; return accum + model.index_buffer_size(); });
    scene.indices_buf = create_buffer(index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    const std::size_t instance_size = scene.num_objects * sizeof(glm::mat4);
    scene.instances_buf = create_buffer(instance_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    const std::size_t indirect_draw_size = scene.num_models * sizeof(VkDrawIndexedIndirectCommand);
    scene.indirect_draw_buf = create_buffer(indirect_draw_size, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    ringbuffer_copy_scene_data_into_buffers(scene, vertex_size, index_size, instance_size, indirect_draw_size);
}

auto RenderContext::cleanup_vulkan_objects_for_scene(RasterScene &scene) noexcept -> void {
    cleanup_buffer(scene.vertices_buf);
    cleanup_buffer(scene.indices_buf);
    cleanup_buffer(scene.instances_buf);
    cleanup_buffer(scene.indirect_draw_buf);
}

auto RenderContext::ringbuffer_copy_scene_data_into_buffers(RasterScene &scene, std::size_t vertex_size, std::size_t index_size, std::size_t instance_size, std::size_t indirect_draw_size) noexcept -> void { 
    char *data_vertex = (char *) ringbuffer_claim_buffer(main_ring_buffer, vertex_size);
    for (const Model &model : scene.models) {
	model.dump_vertices(data_vertex);
	data_vertex += model.vertex_buffer_size();
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.vertices_buf);
    
    char *data_index = (char *) ringbuffer_claim_buffer(main_ring_buffer, index_size);
    for (const Model &model : scene.models) {
	model.dump_indices(data_index);
	data_index += model.index_buffer_size();
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.indices_buf);

    glm::mat4 *data_instance = (glm::mat4 *) ringbuffer_claim_buffer(main_ring_buffer, instance_size);
    for (auto transforms : scene.transforms) {
	memcpy(data_instance, transforms.data(), transforms.size() * sizeof(glm::mat4));
	data_instance += transforms.size();
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.instances_buf);

    VkDrawIndexedIndirectCommand *data_indirect_draw  = (VkDrawIndexedIndirectCommand  *) ringbuffer_claim_buffer(main_ring_buffer, indirect_draw_size);
    uint32_t num_instances_so_far = 0;
    for (std::size_t i = 0; i < scene.num_models; ++i) {
	const std::size_t vertex_buffer_model_offset = scene.model_vertices_offsets[i];
	const std::size_t index_buffer_model_offset = scene.model_indices_offsets[i];
	data_indirect_draw[i].indexCount = scene.models[i].num_indices();
	data_indirect_draw[i].instanceCount = (uint32_t) scene.transforms[i].size();
	data_indirect_draw[i].firstIndex = (uint32_t) index_buffer_model_offset / sizeof(uint16_t);
	data_indirect_draw[i].vertexOffset = (int32_t) vertex_buffer_model_offset / sizeof(Model::Vertex);
	data_indirect_draw[i].firstInstance = num_instances_so_far;
	num_instances_so_far += (uint32_t) scene.transforms[i].size();
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.indirect_draw_buf);
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
    ring_buffer.last_size = size;
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

auto RenderContext::ringbuffer_submit_buffer(RingBuffer &ring_buffer, Buffer dst) noexcept -> void {
    vmaUnmapMemory(allocator, ring_buffer.elements[ring_buffer.last_id].buffer.allocation);

    VkBufferCopy copy_region {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = ring_buffer.last_size;

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

    auto it = ring_buffer.upload_semaphores.find(dst.buffer);
    VkSemaphore prev_semaphore;
    if (it != ring_buffer.upload_semaphores.end()) {
	prev_semaphore = it->second;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &prev_semaphore;
	submit_info.pWaitDstStageMask = wait_stages;
    } else {
	prev_semaphore = create_semaphore();
	ring_buffer.upload_semaphores[dst.buffer] = prev_semaphore;
    }
    VkSemaphore signal_semaphores[] = {prev_semaphore, signal_semaphore};
    submit_info.signalSemaphoreCount = 2;
    submit_info.pSignalSemaphores = signal_semaphores;
    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
}
