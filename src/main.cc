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

#include "context.h"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) noexcept -> int {
    RenderContext context {};
    context.init();

    const Model simple_model1 = {
	{
	    {
		{0.0, -0.5f, -0.5f},
		{1.0f, 0.0f, 0.0f}
	    },
	    {
		{0.0, 0.5f, -0.5f},
		{0.0f, 1.0f, 0.0f}
	    },
	    {
		{0.0, 0.5f, 0.5f},
		{0.0f, 0.0f, 1.0f}
	    },
	    {
		{0.0, -0.5f, 0.5f},
		{1.0f, 1.0f, 1.0f}
	    }
	},
	{0, 1, 2, 2, 3, 0},
    };

    const Model simple_model2 = {
	{
	    {
		{0.0, -0.5f, -0.5f},
		{1.0f, 0.0f, 0.0f}
	    },
	    {
		{0.0, 0.5f, -0.5f},
		{1.0f, 0.0f, 0.0f}
	    },
	    {
		{0.0, 0.5f, 0.5f},
		{1.0f, 0.0f, 0.0f}
	    },
	    {
		{0.0, -0.5f, 0.5f},
		{1.0f, 0.0f, 0.0f}
	    }
	},
	{0, 1, 2, 2, 3, 0},
    };
    
    RasterScene scene {};
    scene.add_model(std::move(simple_model1));
    scene.add_model(std::move(simple_model2));
    scene.add_object(glm::mat4(1), 1);
    scene.add_object(glm::translate(glm::mat4(1), glm::vec3(0.5f, 0.0f, 0.0f)), 0);
    scene.add_object(glm::translate(glm::mat4(1), glm::vec3(-0.5f, 0.0f, 0.0f)), 0);
    scene.add_texture(context.load_texture("models/viking_room.png"));
    context.allocate_vulkan_objects_for_scene(scene);

    [[maybe_unused]] const float aspect_ratio = (float) context.swapchain_extent.width / (float) context.swapchain_extent.height;
    context.perspective_matrix = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 10.0f);
    context.perspective_matrix[1][1] *= -1.0f;

    auto system_time = std::chrono::system_clock::now();
    double rolling_average_dt = 0.0;
    double elapsed_time = 0.0;
    double camera_theta = M_PI / 2.0, camera_phi = M_PI / 4.0;
    float camera_radius = 4.0f;
    const double sensitivity = 100.0;
    while (context.active) {
	const auto current_time = std::chrono::system_clock::now();
	const std::chrono::duration<double> dt_chrono = current_time - system_time;
	const double dt = dt_chrono.count();
	system_time = current_time;
	rolling_average_dt += dt;

	elapsed_time += dt;
	context.camera_matrix = glm::lookAt(camera_radius * glm::vec3(sin(camera_theta) * cos(camera_phi), sin(camera_theta) * sin(camera_phi), cos(camera_theta)), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	context.perspective_camera_matrix = context.perspective_matrix * context.camera_matrix;
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
	}

	scene.transforms[1][0] = glm::translate(glm::mat4(1), glm::vec3(0.0f, sin(elapsed_time), 0.0f));

	context.ringbuffer_copy_scene_instances_into_buffer(scene);
	
	context.render(scene);
	if (context.current_frame % 10000 == 0) {
	    printf("FPS: %f\n", 10000.0 / rolling_average_dt);
	    rolling_average_dt = 0.0;
	}
    }

    vkDeviceWaitIdle(context.device);
    context.cleanup_vulkan_objects_for_scene(scene);
    context.cleanup();
    return 0;
}
