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

    const uint32_t image_count =
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
    vkDestroySwapchainKHR(device, swapchain, NULL);
}
