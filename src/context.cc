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

static auto glfw_framebuffer_resize_callback(GLFWwindow* window, [[maybe_unused]] int width, [[maybe_unused]] int height) noexcept -> void {
    RenderContext *context = (RenderContext*) glfwGetWindowUserPointer(window);
    context->resized = true;
}

static auto glfw_key_callback(GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods) noexcept -> void {
    ASSERT(key >= 0 && key < GLFW_KEY_LAST, "Pressed unknown key.");
    RenderContext *context = (RenderContext*) glfwGetWindowUserPointer(window);
    context->pressed_keys[key] = action != GLFW_RELEASE;
}

auto RenderContext::init() noexcept -> void {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(1000, 1000, "trace", NULL, NULL);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_resize_callback);
    glfwSetKeyCallback(window, glfw_key_callback);

    create_instance();
    create_surface();
    create_physical_device();
    create_device();
    create_allocator();
    create_swapchain();
    create_command_pool();
    create_ray_trace_images();
    create_shaders();
    create_descriptor_pool();
    create_descriptor_set_layout();
    create_descriptor_sets();
    create_ray_trace_descriptor_set_layout();
    create_ray_trace_descriptor_sets();
    create_raster_pipeline();
    create_ray_trace_pipeline();
    create_shader_binding_table();
    create_sampler();
    create_depth_resources();
    create_framebuffers();
    create_command_buffers();
    create_sync_objects();
    main_ring_buffer = create_ringbuffer();
    init_imgui();
}

auto RenderContext::render(Scene &scene) noexcept -> void {
    glfwPollEvents();
    if ((pressed_keys[GLFW_KEY_ESCAPE] && !is_using_imgui()) || glfwWindowShouldClose(window)) {
	active = false;
	return;
    }

    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    if (current_frame == 0) {
	last_mouse_x = mouse_x;
	last_mouse_y = mouse_y;
    }
    for (uint8_t i = 0; i <= GLFW_MOUSE_BUTTON_LAST; ++i)
	pressed_buttons[i] = glfwGetMouseButton(window, i) == GLFW_PRESS;

    render_imgui(scene);

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

    {
	auto it = raster_descriptor_set_writes.find(current_frame);
	while (it != raster_descriptor_set_writes.end()) {
            vkUpdateDescriptorSets(device, 1, &std::get<0>(it->second), 0, NULL);
	    raster_descriptor_set_writes.erase(it);
	    it = raster_descriptor_set_writes.find(current_frame);
	}
    }

    vkResetCommandBuffer(raster_command_buffers[flight_index], 0);
    if (ray_tracing)
	record_ray_trace_command_buffer(raster_command_buffers[flight_index], image_index, flight_index);
    else
	record_raster_command_buffer(raster_command_buffers[flight_index], image_index, flight_index, scene);

    const uint16_t num_wait_semaphores = 1 + main_ring_buffer.get_number_occupied(current_frame);
    ring_buffer_semaphore_scratchpad.reserve(num_wait_semaphores);
    ring_buffer_semaphore_scratchpad.resize(num_wait_semaphores);
    ring_buffer_wait_stages_scratchpad.reserve(num_wait_semaphores);
    ring_buffer_wait_stages_scratchpad.resize(num_wait_semaphores, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    main_ring_buffer.get_new_semaphores(ring_buffer_semaphore_scratchpad.data(), current_frame);
    ring_buffer_semaphore_scratchpad[num_wait_semaphores - 1] = image_available_semaphores[flight_index];
    main_ring_buffer.clear_occupied(current_frame - FRAMES_IN_FLIGHT);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = num_wait_semaphores;
    submit_info.pWaitSemaphores = ring_buffer_semaphore_scratchpad.data();
    submit_info.pWaitDstStageMask = ring_buffer_wait_stages_scratchpad.data();;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &raster_command_buffers[flight_index];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_finished_semaphores[flight_index];

    ASSERT(vkQueueSubmit(queue, 1, &submit_info, in_flight_fences[flight_index]), "");

    VkPresentInfoKHR present_info {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_finished_semaphores[flight_index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &image_index;

    const VkResult queue_present_result = vkQueuePresentKHR(queue, &present_info);
    if (queue_present_result == VK_ERROR_OUT_OF_DATE_KHR || queue_present_result == VK_SUBOPTIMAL_KHR || resized) {
	recreate_swapchain();
    } else {
	ASSERT(queue_present_result, "Unable to present rendered image.");
    }

    for (auto it = buffer_cleanup_queue.begin(); it != buffer_cleanup_queue.end();) {
	if (it->second + FRAMES_IN_FLIGHT == current_frame) {
	    cleanup_buffer(it->first);
	    it = buffer_cleanup_queue.erase(it);
	} else {
	    ++it;
	}
    }

    resized = false;
    ++current_frame;
}

auto RenderContext::cleanup() noexcept -> void {
    for (auto [buffer, _] : buffer_cleanup_queue) {
	cleanup_buffer(buffer);
    }
    cleanup_imgui();
    cleanup_ringbuffer(main_ring_buffer);
    cleanup_sync_objects();
    cleanup_framebuffers();
    cleanup_depth_resources();
    cleanup_descriptor_pool();
    cleanup_descriptor_set_layout();
    cleanup_ray_trace_descriptor_set_layout();
    cleanup_shader_binding_table();
    cleanup_ray_trace_pipeline();
    cleanup_raster_pipeline();
    cleanup_sampler();
    cleanup_shaders();
    cleanup_ray_trace_images();
    cleanup_command_pool();
    cleanup_swapchain();
    cleanup_allocator();
    cleanup_device();
    cleanup_surface();
    cleanup_instance();
    glfwDestroyWindow(window);
    glfwTerminate();
}
