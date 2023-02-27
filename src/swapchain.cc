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

auto RenderContext::create_swapchain() noexcept -> void {
    ZoneScoped;
    const SwapchainSupport support = physical_check_swapchain_support(physical_device);
    ASSERT(support.formats.size() > 0 && support.present_modes.size() > 0, "Swapchain support is suddenly not available for chosen physical device.");

    const auto [surface_format, present_mode, swap_extent] = choose_swapchain_options(support);

    uint32_t image_count =
	support.capabilities.maxImageCount > 0 && support.capabilities.minImageCount >= support.capabilities.maxImageCount ?
	support.capabilities.maxImageCount :
	support.capabilities.minImageCount + 1;

    VkSwapchainCreateInfoKHR create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = swap_extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform = support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    ASSERT(vkCreateSwapchainKHR(device, &create_info, NULL, &swapchain), "Couldn't create swapchain.");

    vkGetSwapchainImagesKHR(device, swapchain, &image_count, NULL);
    swapchain_images.resize(image_count);
    swapchain_image_views.resize(image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, &swapchain_images[0]);
    swapchain_format = surface_format.format;
    swapchain_extent = swap_extent;

    VkImageSubresourceRange subresource_range {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;
    for (uint32_t image_index = 0; image_index < image_count; ++image_index) {
	swapchain_image_views[image_index] = create_image_view(swapchain_images[image_index], swapchain_format, subresource_range);
    }
}

auto RenderContext::choose_swapchain_options(const SwapchainSupport &support) noexcept -> std::tuple<VkSurfaceFormatKHR, VkPresentModeKHR, VkExtent2D> {
    ZoneScoped;
    VkSurfaceFormatKHR surface_format {};
    VkPresentModeKHR present_mode {};
    VkExtent2D swap_extent {};
    
    uint32_t format_index = 0;
    for (; format_index < support.formats.size(); ++format_index) {
	if (support.formats[format_index].format == VK_FORMAT_B8G8R8A8_SRGB && support.formats[format_index].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
	    surface_format = support.formats[format_index];
	    break;
	}
    }
    ASSERT(format_index < support.formats.size(), "ERROR: Queried surface format not found to be available.");

    present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t present_mode_index = 0; present_mode_index < support.present_modes.size(); ++present_mode_index) {
	if (support.present_modes[present_mode_index] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
	    present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	}
	else if (support.present_modes[present_mode_index] == VK_PRESENT_MODE_MAILBOX_KHR) {
	    present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
	    break;
	}
    }

    if (support.capabilities.currentExtent.width != UINT32_MAX) {
	swap_extent = support.capabilities.currentExtent;
    }
    else {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	
	swap_extent.width = (uint32_t) width;
	swap_extent.height = (uint32_t) height;

	swap_extent.width = swap_extent.width < support.capabilities.minImageExtent.width ? support.capabilities.minImageExtent.width : swap_extent.width;
	swap_extent.height = swap_extent.height < support.capabilities.minImageExtent.height ? support.capabilities.minImageExtent.height : swap_extent.height;
	swap_extent.width = swap_extent.width > support.capabilities.maxImageExtent.width ? support.capabilities.maxImageExtent.width : swap_extent.width;
	swap_extent.height = swap_extent.height > support.capabilities.maxImageExtent.height ? support.capabilities.maxImageExtent.height : swap_extent.height;
    }

    return {surface_format, present_mode, swap_extent};
}

auto RenderContext::cleanup_swapchain() noexcept -> void {
    ZoneScoped;
    for (auto view : swapchain_image_views) {
	vkDestroyImageView(device, view, NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, NULL);
}

auto RenderContext::create_ray_trace_images() noexcept -> void {
    ZoneScoped;
    VkImageSubresourceRange subresource_range {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;

    VkFormat ray_trace_formats[6] = {
	VK_FORMAT_R8G8B8A8_UNORM,
	VK_FORMAT_R32G32B32A32_SFLOAT,
	VK_FORMAT_R32G32B32A32_SFLOAT,
	VK_FORMAT_R16G16B16A16_SFLOAT,
	VK_FORMAT_R8G8B8A8_UNORM,
	VK_FORMAT_R32G32B32A32_SFLOAT,
    };

    for (uint32_t i = 0; i < sizeof(ray_trace_formats) / sizeof(ray_trace_formats[0]); ++i) {
	ray_trace1_images[i] = create_image(0, ray_trace_formats[i], swapchain_extent, 1, 1, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "RAY_TRACING_STORAGE_IMAGE");
	ray_trace1_image_views[i] = create_image_view(ray_trace1_images[i].image, ray_trace_formats[i], subresource_range);
	ray_trace2_images[i] = create_image(0, ray_trace_formats[i], swapchain_extent, 1, 1, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "RAY_TRACING_STORAGE_IMAGE");
	ray_trace2_image_views[i] = create_image_view(ray_trace2_images[i].image, ray_trace_formats[i], subresource_range);
    }

    for (uint32_t i = 0; i < 2; ++i) {
	taa_images[i] = create_image(0, VK_FORMAT_R32G32B32A32_SFLOAT, swapchain_extent, 1, 1, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "TAA_STORAGE_IMAGE");
	taa_image_views[i] = create_image_view(taa_images[i].image, VK_FORMAT_R32G32B32A32_SFLOAT, subresource_range);
    }

    inefficient_run_commands([&](VkCommandBuffer cmd) {
	VkImageMemoryBarrier image_memory_barrier {};
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barrier.subresourceRange.baseMipLevel = 0;
	image_memory_barrier.subresourceRange.levelCount = 1;
	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	image_memory_barrier.subresourceRange.layerCount = 1;
	image_memory_barrier.srcAccessMask = 0;
	image_memory_barrier.dstAccessMask = 0;

	for (uint32_t i = 0; i < sizeof(ray_trace_formats) / sizeof(ray_trace_formats[0]); ++i) {
	    image_memory_barrier.image = ray_trace1_images[i].image;
	    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
	    image_memory_barrier.image = ray_trace2_images[i].image;
	    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
	}

	for (uint32_t i = 0; i < 2; ++i) {
	    image_memory_barrier.image = taa_images[i].image;
	    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
	}
    });

    motion_vector_image = create_image(0, VK_FORMAT_R32G32_SFLOAT, swapchain_extent, 1, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "MOTION_VECTORS_IMAGE");
    motion_vector_image_view = create_image_view(motion_vector_image.image, VK_FORMAT_R32G32_SFLOAT, subresource_range);

    subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    motion_vector_depth_image = create_image(0, VK_FORMAT_D32_SFLOAT, swapchain_extent, 1, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, "MOTION_VECTORS_IMAGE");
    motion_vector_depth_image_view = create_image_view(motion_vector_depth_image.image, VK_FORMAT_D32_SFLOAT, subresource_range);
}

auto RenderContext::cleanup_ray_trace_images() noexcept -> void {
    ZoneScoped;
    for (uint32_t i = 0; i < ray_trace1_images.size(); ++i) {
	cleanup_image_view(ray_trace1_image_views[i]);
	cleanup_image(ray_trace1_images[i]);
    }
    for (uint32_t i = 0; i < ray_trace2_images.size(); ++i) {
	cleanup_image_view(ray_trace2_image_views[i]);
	cleanup_image(ray_trace2_images[i]);
    }
    
    cleanup_image(motion_vector_image);
    cleanup_image_view(motion_vector_image_view);

    cleanup_image(motion_vector_depth_image);
    cleanup_image_view(motion_vector_depth_image_view);

    for (uint32_t i = 0; i < 2; ++i) {
	cleanup_image(taa_images[i]);
	cleanup_image_view(taa_image_views[i]);
    }
}

auto RenderContext::recreate_swapchain() noexcept -> void {
    ZoneScoped;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(device);

    cleanup_framebuffers();
    cleanup_swapchain();
    cleanup_ray_trace_images();
    
    create_ray_trace_images();
    create_swapchain();
    create_framebuffers();

    update_descriptors_ray_trace_images();
    update_descriptors_motion_vector_texture();
    update_descriptors_taa_images();

    recreate_imgui();
}
