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

static std::size_t num_heap_allocs = 0;
void *operator new(size_t size) {
    num_heap_allocs++;
    return malloc(size);
}

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) noexcept -> int {
    srand(time(NULL));
    
    RenderContext context {};
    context.init();
    context.update_descriptors_ray_trace_images();

    Scene scene {};

    /*const uint16_t model_id_pico = context.load_model("pico", scene);
    for (int16_t x = -5; x <= 5; ++x)
	for (int16_t y = -5; y <= 5; ++y)
	scene.add_object(glm::scale(glm::translate(glm::mat4(1), glm::vec3(x * 1.2f, y * 1.2f, 0.7f)), glm::vec3(20.0f, 20.0f, 20.0f)), model_id_pico);
    const uint16_t model_id_stone_lion = context.load_model("stone_lion", scene);
    for (int16_t x = -5; x <= 5; ++x)
	for (int16_t y = -5; y <= 5; ++y)
	scene.add_object(glm::translate(glm::mat4(1), glm::vec3(x * 1.2f, y * 1.2f, -0.7f)), model_id_stone_lion);
    scene.add_light({0.0, 10.0, 0.0, 100.0});*/

    scene.add_light({3.0, 3.0, 6.0, 50.0});
    scene.add_light({3.0, -3.0, 6.0, 50.0});

    const uint16_t model_id_dragon = context.load_model("dragon", scene);
    scene.add_object(glm::scale(glm::translate(glm::mat4(1), glm::vec3(-2.5f, 0.0f, 0.0f)), glm::vec3(0.05f, 0.05f, 0.05f)), model_id_dragon);

    const uint8_t mat_red[] = {220, 80, 100, 255, 0};
    const uint16_t model_id_red_dragon = context.load_model("dragon", scene, &mat_red[0]);
    scene.add_object(glm::scale(glm::rotate(glm::translate(glm::mat4(1), glm::vec3(0.0f, 5.5f, 0.0f)), -1.0f, glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(0.05f, 0.05f, 0.05f)), model_id_red_dragon);

    const uint8_t mat_blue[] = {150, 220, 255, 75, 255};
    const uint16_t model_id_blue_dragon = context.load_model("dragon", scene, &mat_blue[0]);
    scene.add_object(glm::scale(glm::rotate(glm::translate(glm::mat4(1), glm::vec3(0.0f, -5.5f, 0.0f)), 1.0f, glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(0.05f, 0.05f, 0.05f)), model_id_blue_dragon);

    const uint16_t model_id_floor = context.load_custom_model(
							      {
								  {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
								  {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
								  {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
								  {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
							      },
        						      {
								  0, 1, 2,
								  1, 3, 2,
							      },
							      255,
							      255,
							      255,
							      0,
							      0,
							      scene
							      );
    scene.add_object(glm::scale(glm::mat4(1), glm::vec3(10.0f, 10.0f, 1.0f)), model_id_floor);
    
    context.allocate_vulkan_objects_for_scene(scene);
    context.update_descriptors_lights(scene);
    context.update_descriptors_perspective();
    context.build_bottom_level_acceleration_structure_for_model(model_id_dragon, scene);
    context.build_bottom_level_acceleration_structure_for_model(model_id_red_dragon, scene);
    context.build_bottom_level_acceleration_structure_for_model(model_id_blue_dragon, scene);
    context.build_bottom_level_acceleration_structure_for_model(model_id_floor, scene);
    context.build_top_level_acceleration_structure_for_scene(scene);
    context.update_descriptors_tlas(scene);
    context.update_descriptors_ray_trace_objects(scene);
    
    //const float aspect_ratio = (float) context.swapchain_extent.width / (float) context.swapchain_extent.height;
    context.camera_position = glm::vec3(3.0f, 3.0f, 4.0f);

    auto system_time = std::chrono::system_clock::now();
    double elapsed_time_subsecond = 0.0;
    uint32_t num_frames_subsecond = 0;
    double elapsed_time = 0.0;
    double camera_theta = 3.0 * M_PI / 4.0, camera_phi = 5.0 * M_PI / 4.0;
    const double sensitivity = 100.0, move_speed = 5.0;
    while (context.active) {
	const auto current_time = std::chrono::system_clock::now();
	const std::chrono::duration<double> dt_chrono = current_time - system_time;
	const double dt = dt_chrono.count();
	system_time = current_time;
	elapsed_time += dt;
	elapsed_time_subsecond += dt;
	++num_frames_subsecond;

	const glm::vec3 view_dir = glm::vec3(sin(camera_theta) * cos(camera_phi), sin(camera_theta) * sin(camera_phi), cos(camera_theta));
	context.push_constants.camera_matrix = glm::lookAt(context.camera_position, context.camera_position + view_dir, glm::vec3(0.0f, 0.0f, 1.0f));
	context.push_constants.seed = rand();
	if (!context.is_using_imgui()) {
	    const double mouse_dx = context.mouse_x - context.last_mouse_x;
	    const double mouse_dy = context.mouse_y - context.last_mouse_y;
	    if (context.pressed_buttons[GLFW_MOUSE_BUTTON_LEFT] == GLFW_PRESS) {
		camera_phi -= mouse_dx / sensitivity;
		camera_theta += mouse_dy / sensitivity;
		camera_theta = glm::clamp(camera_theta, 0.01, M_PI - 0.01);
		if (camera_phi < 0.0) camera_phi += 2.0 * M_PI;
		if (camera_phi >= 2.0 * M_PI) camera_phi -= 2.0 * M_PI;
	    }
	    if (context.pressed_keys[GLFW_KEY_W] == GLFW_PRESS) {
		context.camera_position += (float) (dt * move_speed) * glm::vec3(cos(camera_phi), sin(camera_phi), 0.0);
	    }
	    if (context.pressed_keys[GLFW_KEY_A] == GLFW_PRESS) {
		context.camera_position += (float) (dt * move_speed) * glm::vec3(-sin(camera_phi), cos(camera_phi), 0.0);
	    }
	    if (context.pressed_keys[GLFW_KEY_S] == GLFW_PRESS) {
		context.camera_position += (float) (dt * move_speed) * glm::vec3(-cos(camera_phi), -sin(camera_phi), 0.0);
	    }
	    if (context.pressed_keys[GLFW_KEY_D] == GLFW_PRESS) {
		context.camera_position += (float) (dt * move_speed) * glm::vec3(sin(camera_phi), -cos(camera_phi), 0.0);
	    }
	    if (context.pressed_keys[GLFW_KEY_LEFT_SHIFT] == GLFW_PRESS) {
		context.camera_position += (float) (dt * move_speed) * glm::vec3(0.0, 0.0, -1.0);
	    }
	    if (context.pressed_keys[GLFW_KEY_SPACE] == GLFW_PRESS) {
		context.camera_position += (float) (dt * move_speed) * glm::vec3(0.0, 0.0, 1.0);
	    }
	    if (context.pressed_keys[GLFW_KEY_R] == GLFW_PRESS && context.last_pressed_keys[GLFW_KEY_R] != GLFW_PRESS) {
		context.ray_tracing = !context.ray_tracing;
	    }
	}
	context.last_pressed_keys = context.pressed_keys;

	/*scene.transforms[model_id_dragon][0] = glm::scale(glm::rotate(glm::translate(glm::mat4(1), glm::vec3(-2.5f, 0.0f, 0.0f)), (float) elapsed_time, glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(0.05f, 0.05f, 0.05f));
	
	context.ringbuffer_copy_scene_instances_into_buffer(scene);
	context.ringbuffer_copy_scene_lights_into_buffer(scene);*/
	
	context.render(scene);
	
	if (elapsed_time_subsecond >= 0.25f) {
	    const float fps = (float) num_frames_subsecond / (float) elapsed_time_subsecond;
	    elapsed_time_subsecond = 0.0;
	    num_frames_subsecond = 0;
	    for (uint16_t i = 0; i < context.imgui_data.last_fpss.size() - 1; ++i) {
		context.imgui_data.last_fpss[i] = context.imgui_data.last_fpss[i + 1];
	    }
	    context.imgui_data.last_fpss[context.imgui_data.last_fpss.size() - 1] = fps;
	}
	for (uint16_t i = 0; i < context.imgui_data.last_heaps.size() - 1; ++i) {
	    context.imgui_data.last_heaps[i] = context.imgui_data.last_heaps[i + 1];
	}
	context.imgui_data.last_heaps[context.imgui_data.last_heaps.size() - 1] = (float) num_heap_allocs;
	num_heap_allocs = 0;
    }

    vkDeviceWaitIdle(context.device);
    context.cleanup_vulkan_objects_for_scene(scene);
    context.cleanup();
    return 0;
}
