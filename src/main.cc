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

#include <iostream>
#include <chrono>

#include <glm/gtx/transform.hpp>

#include "context.h"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) noexcept -> int {
    RenderContext context {};
    context.init();

    RasterScene scene {};

    const uint16_t model_id_stone_lion = context.load_model("stone_lion", scene);
    for (int16_t x = -5; x <= 5; ++x)
	for (int16_t y = -5; y <= 5; ++y)
	    scene.add_object(glm::translate(glm::mat4(1), glm::vec3(x * 1.2f, y * 1.2f, 0.0f)), model_id_stone_lion);

    const uint16_t model_id_pico = context.load_model("pico", scene);
    for (int16_t x = -5; x <= 5; ++x)
	for (int16_t y = -5; y <= 5; ++y)
	    scene.add_object(glm::translate(glm::mat4(1), glm::vec3(x * 0.05f, y * 0.05f, 0.7f)), model_id_pico);

    scene.add_light({0.0, 2.0, 2.0, 10.0});
    
    context.allocate_vulkan_objects_for_scene(scene);
    context.update_descriptors_lights(scene);
    
    const float aspect_ratio = (float) context.swapchain_extent.width / (float) context.swapchain_extent.height;
    context.perspective_matrix = glm::perspective(glm::radians(45.0f), 1.0f, 0.01f, 1000.0f);
    context.perspective_matrix[1][1] *= -1.0f;

    auto system_time = std::chrono::system_clock::now();
    double elapsed_time_subsecond = 0.0;
    uint32_t num_frames_subsecond = 0;
    double elapsed_time = 0.0;
    double camera_theta = M_PI / 4.0, camera_phi = M_PI / 4.0;
    float camera_radius = 4.0f;
    const double sensitivity = 100.0;
    while (context.active) {
	const auto current_time = std::chrono::system_clock::now();
	const std::chrono::duration<double> dt_chrono = current_time - system_time;
	const double dt = dt_chrono.count();
	system_time = current_time;
	elapsed_time += dt;
	elapsed_time_subsecond += dt;
	++num_frames_subsecond;

	context.camera_position = camera_radius * glm::vec3(sin(camera_theta) * cos(camera_phi), sin(camera_theta) * sin(camera_phi), cos(camera_theta));
	context.camera_matrix = glm::lookAt(context.camera_position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	context.perspective_camera_matrix = context.perspective_matrix * context.camera_matrix;
	if (!context.is_using_imgui()) {
	    const double mouse_dx = context.mouse_x - context.last_mouse_x;
	    const double mouse_dy = context.mouse_y - context.last_mouse_y;
	    if (context.pressed_buttons[GLFW_MOUSE_BUTTON_LEFT] == GLFW_PRESS) {
		camera_phi -= mouse_dx / sensitivity;
		camera_theta -= mouse_dy / sensitivity;
		camera_theta = glm::clamp(camera_theta, 0.01, M_PI - 0.01);
		if (camera_phi < 0.0) camera_phi += 2.0 * M_PI;
		if (camera_phi >= 2.0 * M_PI) camera_phi -= 2.0 * M_PI;
	    }
	    if (context.pressed_buttons[GLFW_MOUSE_BUTTON_RIGHT] == GLFW_PRESS) {
		camera_radius += (float) mouse_dy / (float) sensitivity;
		if (camera_radius < 0.0001) camera_radius = 0.0001f;
	    }
	}

	uint32_t idx = 0;
	for (int16_t x = -5; x <= 5; ++x) {
	    for (int16_t y = -5; y <= 5; ++y) {
		scene.transforms[model_id_stone_lion][idx] = glm::rotate(scene.transforms[model_id_stone_lion][idx], (float) dt, glm::vec3(x, y, 1.0f));
		++idx;
	    }
	}

	idx = 0;
	for (int16_t x = -5; x <= 5; ++x) {
	    for (int16_t y = -5; y <= 5; ++y) {
		scene.transforms[model_id_pico][idx] = glm::rotate(scene.transforms[model_id_pico][idx], (float) dt, glm::vec3(y, -x, 1.0f));
		++idx;
	    }
	}
	
	context.ringbuffer_copy_scene_instances_into_buffer(scene);
	context.ringbuffer_copy_scene_lights_into_buffer(scene);
	
	context.render(scene);
	
	if (elapsed_time_subsecond >= 1.0f) {
	    printf("FPS: %f\n", (float) num_frames_subsecond / elapsed_time_subsecond);
	    elapsed_time_subsecond = 0.0;
	    num_frames_subsecond = 0;
	}
    }

    vkDeviceWaitIdle(context.device);
    context.cleanup_vulkan_objects_for_scene(scene);
    context.cleanup();
    return 0;
}
