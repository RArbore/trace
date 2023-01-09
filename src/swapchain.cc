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

auto RenderContext::create_swapchain() noexcept -> void {
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
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform = support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    ASSERT(vkCreateSwapchainKHR(device, &create_info, NULL, &swapchain), "Couldn't create swapchain.");

    vkGetSwapchainImagesKHR(device, swapchain, &image_count, NULL);
    swapchain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, &swapchain_images[0]);
    swapchain_format = surface_format.format;
    swapchain_extent = swap_extent;

    VkImageViewCreateInfo image_view_create_info {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = swapchain_format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    swapchain_image_views.resize(image_count);
    for (uint32_t image_index = 0; image_index < image_count; ++image_index) {
	image_view_create_info.image = swapchain_images[image_index];
	ASSERT(vkCreateImageView(device, &image_view_create_info, NULL, &swapchain_image_views[image_index]), "Couldn't create swapchain image view.");
    }
}

auto RenderContext::choose_swapchain_options(const SwapchainSupport &support) noexcept -> std::tuple<VkSurfaceFormatKHR, VkPresentModeKHR, VkExtent2D> {
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
    for (auto view : swapchain_image_views) {
	vkDestroyImageView(device, view, NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, NULL);
}

auto RenderContext::recreate_swapchain() noexcept -> void {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(device);

    cleanup_framebuffers();
    cleanup_depth_resources();
    cleanup_swapchain();
    
    create_swapchain();
    create_depth_resources();
    create_framebuffers();
}
