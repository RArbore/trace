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

#include <tiny_obj_loader.h>

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
    return {buffer, allocation, size};
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

    ringbuffer_copy_scene_vertices_into_buffer(scene);
    ringbuffer_copy_scene_indices_into_buffer(scene);
    ringbuffer_copy_scene_instances_into_buffer(scene);
    ringbuffer_copy_scene_indirect_draw_into_buffer(scene);
}

auto RenderContext::cleanup_vulkan_objects_for_scene(RasterScene &scene) noexcept -> void {
    cleanup_buffer(scene.vertices_buf);
    cleanup_buffer(scene.indices_buf);
    cleanup_buffer(scene.instances_buf);
    cleanup_buffer(scene.indirect_draw_buf);
    for (auto image : scene.textures) {
	cleanup_image_view(image.second);
	cleanup_image(image.first);
    }
}

auto RenderContext::ringbuffer_copy_scene_vertices_into_buffer(RasterScene &scene) noexcept -> void {
    char *data_vertex = (char *) ringbuffer_claim_buffer(main_ring_buffer, scene.vertices_buf.size);
    for (const Model &model : scene.models) {
	model.dump_vertices(data_vertex);
	data_vertex += model.vertex_buffer_size();
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.vertices_buf);
}

auto RenderContext::ringbuffer_copy_scene_indices_into_buffer(RasterScene &scene) noexcept -> void {
    char *data_index = (char *) ringbuffer_claim_buffer(main_ring_buffer, scene.indices_buf.size);
    for (const Model &model : scene.models) {
	model.dump_indices(data_index);
	data_index += model.index_buffer_size();
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.indices_buf);
}

auto RenderContext::ringbuffer_copy_scene_instances_into_buffer(RasterScene &scene) noexcept -> void {
    glm::mat4 *data_instance = (glm::mat4 *) ringbuffer_claim_buffer(main_ring_buffer, scene.instances_buf.size);
    for (auto transforms : scene.transforms) {
	memcpy(data_instance, transforms.data(), transforms.size() * sizeof(glm::mat4));
	data_instance += transforms.size();
    }
    ringbuffer_submit_buffer(main_ring_buffer, scene.instances_buf);
}

auto RenderContext::ringbuffer_copy_scene_indirect_draw_into_buffer(RasterScene &scene) noexcept -> void {
    VkDrawIndexedIndirectCommand *data_indirect_draw  = (VkDrawIndexedIndirectCommand  *) ringbuffer_claim_buffer(main_ring_buffer, scene.indirect_draw_buf.size);
    uint32_t num_instances_so_far = 0;
    for (std::size_t i = 0; i < scene.num_models; ++i) {
	const std::size_t vertex_buffer_model_offset = scene.model_vertices_offsets[i];
	const std::size_t index_buffer_model_offset = scene.model_indices_offsets[i];
	data_indirect_draw[i].indexCount = scene.models[i].num_indices();
	data_indirect_draw[i].instanceCount = (uint32_t) scene.transforms[i].size();
	data_indirect_draw[i].firstIndex = (uint32_t) index_buffer_model_offset / sizeof(uint32_t);
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
    copy_region.size = dst.size;

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

auto RenderContext::load_texture(const char *filepath) noexcept -> std::pair<Image, VkImageView> {
    int tex_width, tex_height, tex_channels;
    stbi_uc* pixels = stbi_load(filepath, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    ASSERT(pixels, "Unable to load texture.");
    std::size_t image_size = tex_width * tex_height * 4;

    VkExtent2D extent = {(uint32_t) tex_width, (uint32_t) tex_height};
    Image dst = create_image(0, VK_FORMAT_R8G8B8A8_SRGB, extent, 1, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    void *data_image = ringbuffer_claim_buffer(main_ring_buffer, image_size);
    memcpy(data_image, pixels, image_size);
    ringbuffer_submit_buffer(main_ring_buffer, dst);

    VkImageSubresourceRange subresource_range {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;

    return {dst, create_image_view(dst.image, VK_FORMAT_R8G8B8A8_SRGB, subresource_range)};
}

auto RenderContext::load_singleton_scene(const char *obj_filepath, const char *texture_filepath) noexcept -> RasterScene {
    RasterScene scene {};

    scene.add_model(load_obj_model(obj_filepath));
    scene.add_object(glm::mat4(1), 0);
    scene.add_texture(load_texture(texture_filepath));

    return scene;
}

auto RenderContext::load_obj_model(const char *obj_filepath) noexcept -> Model {
    Model model {};

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, obj_filepath), "Unable to load OBJ model.");

    for (const auto& shape : shapes) {
	for (const auto& index : shape.mesh.indices) {
	    Model::Vertex vertex {};

	    vertex.position = {
		attrib.vertices[3 * index.vertex_index + 0],
		attrib.vertices[3 * index.vertex_index + 1],
		attrib.vertices[3 * index.vertex_index + 2]
	    };
	    
	    vertex.texture = {
		attrib.texcoords[2 * index.texcoord_index + 0],
		1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
	    };
	    
	    vertex.color = {1.0f, 1.0f, 1.0f};
	    
	    model.vertices.push_back(vertex);
	    model.indices.push_back((uint32_t) model.indices.size());
	}
    }
    
    return model;
}
