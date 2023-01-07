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

static auto glfw_framebuffer_resize_callback(GLFWwindow* window, int width, int height) noexcept -> void {
    RenderContext *context = (RenderContext*) glfwGetWindowUserPointer(window);
    context->width = width;
    context->height = height;
}

static auto glfw_key_callback(GLFWwindow* window, int key, __attribute__((unused)) int scancode, int action, __attribute__((unused)) int mods) noexcept -> void {
    ASSERT(key >= 0 && key < GLFW_KEY_LAST, "Pressed unknown key.");
    RenderContext *context = (RenderContext*) glfwGetWindowUserPointer(window);
    context->pressed_keys[key] = action != GLFW_RELEASE;
}

auto RenderContext::init() noexcept -> void {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, "trace", NULL, NULL);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_resize_callback);
    glfwSetKeyCallback(window, glfw_key_callback);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    create_instance();
    create_surface();
    create_physical_device();
    create_device();
    create_swapchain();
    create_shaders();
    create_raster_pipeline();
    create_framebuffers();
    create_command_pool();
    create_command_buffers();
    create_sync_objects();
}

auto RenderContext::render() noexcept -> void {
    glfwPollEvents();
    if (pressed_keys[GLFW_KEY_ESCAPE] || glfwWindowShouldClose(window)) {
	active = false;
	return;
    }

    const uint32_t flight_index = current_frame % FRAMES_IN_FLIGHT;

    vkWaitForFences(device, 1, &in_flight_fences[flight_index], VK_TRUE, UINT64_MAX);

    uint32_t image_index;
    const VkResult acquire_next_image_result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphores[flight_index], VK_NULL_HANDLE, &image_index);
    if (acquire_next_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
	recreate_swapchain();
	return;
    }
    ASSERT(acquire_next_image_result == VK_SUCCESS || acquire_next_image_result == VK_SUBOPTIMAL_KHR, "Unable to acquire next image.");
    vkResetFences(device, 1, &in_flight_fences[flight_index]);

    vkResetCommandBuffer(raster_command_buffers[flight_index], 0);
    record_raster_command_buffer(raster_command_buffers[flight_index], image_index);

    const VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_available_semaphores[flight_index];
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &raster_command_buffers[flight_index];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_finished_semaphores[flight_index];

    ASSERT(vkQueueSubmit(queue, 1, &submit_info, in_flight_fences[flight_index]), "");

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_finished_semaphores[flight_index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &image_index;

    const VkResult queue_present_result = vkQueuePresentKHR(queue, &present_info);
    if (queue_present_result == VK_ERROR_OUT_OF_DATE_KHR || queue_present_result == VK_SUBOPTIMAL_KHR) {
	recreate_swapchain();
    } else {
	ASSERT(queue_present_result, "Unable to present rendered image.");
    }

    ++current_frame;
}

auto RenderContext::recreate_swapchain() noexcept -> void {
    vkDeviceWaitIdle(device);

    cleanup_framebuffers();
    cleanup_swapchain();
    
    create_swapchain();
    create_framebuffers();
}

auto RenderContext::cleanup() noexcept -> void {
    vkDeviceWaitIdle(device);

    cleanup_sync_objects();
    cleanup_command_pool();
    cleanup_framebuffers();
    cleanup_raster_pipeline();
    cleanup_shaders();
    cleanup_swapchain();
    cleanup_device();
    cleanup_surface();
    cleanup_instance();
    glfwDestroyWindow(window);
    glfwTerminate();
}
