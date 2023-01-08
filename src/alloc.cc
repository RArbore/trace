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

auto RenderContext::create_image(VkImageCreateFlags flags, VkFormat format, VkExtent3D extent, uint32_t mipLevels, uint32_t arrayLevels, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_flags, VmaAllocationCreateFlags vma_flags) noexcept -> Image {
    VkImageCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.flags = flags;
    create_info.imageType = extent.depth > 1 ? VK_IMAGE_TYPE_3D : extent.height > 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_1D;
    create_info.format = format;
    create_info.extent = extent;
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

auto RenderContext::create_image_view(VkImage image, VkImageViewType type, VkFormat format, VkImageSubresourceRange subresource_range) noexcept -> VkImageView {
    VkImageViewCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    create_info.viewType = type;
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

auto RenderContext::allocate_vulkan_objects_for_model(Model &model) noexcept -> void {
    const std::size_t vertex_size = model.positions_buffer_size() + model.colors_buffer_size();
    model.vertices_buf = create_buffer(vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    const std::size_t index_size = model.indices_buffer_size();
    model.indices_buf = create_buffer(index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

auto RenderContext::cleanup_vulkan_objects_for_model(Model &model) noexcept -> void {
    cleanup_buffer(model.vertices_buf);
    cleanup_buffer(model.indices_buf);
}

auto RenderContext::inefficient_copy_into_buffer(Buffer dst, const std::vector<glm::vec3> &src1, const std::vector<glm::vec3> &src2) noexcept -> void {
    std::size_t src1_size = src1.size() * sizeof(glm::vec3);
    std::size_t src2_size = src2.size() * sizeof(glm::vec3);
    Buffer cpu_visible = create_buffer(src1_size + src2_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    char *data;
    vmaMapMemory(allocator, cpu_visible.allocation, (void **) &data);
    memcpy(data, src1.data(), src1_size);
    memcpy(data + src1_size, src2.data(), src2_size);
    vmaUnmapMemory(allocator, cpu_visible.allocation);

    VkCommandBufferAllocateInfo command_buffer_alloc_info {};
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandPool = command_pool;
    command_buffer_alloc_info.commandBufferCount = 1;
    
    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &command_buffer_alloc_info, &command_buffer);

    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

    VkBufferCopy copy_region {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = src1_size + src2_size;
    vkCmdCopyBuffer(command_buffer, cpu_visible.buffer, dst.buffer, 1, &copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    
    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
    
    cleanup_buffer(cpu_visible);
}

auto RenderContext::inefficient_copy_into_buffer(Buffer dst, const std::vector<uint16_t> &src) noexcept -> void {
    std::size_t src_size = src.size() * sizeof(uint16_t);
    Buffer cpu_visible = create_buffer(src_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    char *data;
    vmaMapMemory(allocator, cpu_visible.allocation, (void **) &data);
    memcpy(data, src.data(), src_size);
    vmaUnmapMemory(allocator, cpu_visible.allocation);

    VkCommandBufferAllocateInfo command_buffer_alloc_info {};
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandPool = command_pool;
    command_buffer_alloc_info.commandBufferCount = 1;
    
    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &command_buffer_alloc_info, &command_buffer);

    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

    VkBufferCopy copy_region {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = src_size;
    vkCmdCopyBuffer(command_buffer, cpu_visible.buffer, dst.buffer, 1, &copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    
    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
    
    cleanup_buffer(cpu_visible);
}
