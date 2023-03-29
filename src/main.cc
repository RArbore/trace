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

#include "Tracy.hpp"

#include "context.h"

static std::size_t num_heap_allocs = 0;
auto operator new(size_t size) -> void * {
    num_heap_allocs++;
    return malloc(size);
}

auto main([[maybe_unused]] int32_t argc, [[maybe_unused]] char **argv) noexcept -> int32_t {
    ZoneScoped;
    srand((uint32_t) time(NULL));
    
    RenderContext context {};
    context.init();

    Scene scene {};

    scene.add_light({10.0, 2.0, 7.0, 2000.0});

    const uint16_t model_id_dragon = context.load_model("dragon", scene);
    scene.add_object(glm::scale(glm::translate(glm::mat4(1), glm::vec3(-2.5f, 0.0f, 0.0f)), glm::vec3(0.05f, 0.05f, 0.05f)), model_id_dragon);

    const uint8_t mat_red[] = {220, 80, 100, 255, 0};
    const uint16_t model_id_red_dragon = context.load_model("dragon", scene, &mat_red[0]);
    scene.add_object(glm::scale(glm::rotate(glm::translate(glm::mat4(1), glm::vec3(0.0f, 5.5f, 0.0f)), -1.0f, glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(0.05f, 0.05f, 0.05f)), model_id_red_dragon);

    const uint8_t mat_blue[] = {150, 220, 255, 75, 255};
    const uint16_t model_id_blue_dragon = context.load_model("dragon", scene, &mat_blue[0]);
    scene.add_object(glm::scale(glm::rotate(glm::translate(glm::mat4(1), glm::vec3(0.0f, -5.5f, 0.0f)), 1.0f, glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(0.05f, 0.05f, 0.05f)), model_id_blue_dragon);

    const uint16_t model_id_pico = context.load_model("pico", scene);
    scene.add_object(glm::scale(glm::translate(glm::mat4(1), glm::vec3(2.0f, -2.0f, 4.0f)), glm::vec3(50.0f)), model_id_pico);

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
							      200,
							      200,
							      200,
							      200,
							      200,
							      scene
							      );
    scene.add_object(glm::scale(glm::mat4(1), glm::vec3(10.0f, 10.0f, 1.0f)), model_id_floor);
    
    const uint16_t model_id_wall = context.load_custom_model(
							      {
								  {{-1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        							  {{-1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
								  {{-1.0f, -1.0f, 2.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
								  {{-1.0f, 1.0f, 2.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
							      },
        						      {
								  0, 1, 2,
								  1, 3, 2,
							      },
							      250,
							      150,
							      250,
							      200,
							      20,
							      scene
							      );
    scene.add_object(glm::scale(glm::mat4(1), glm::vec3(10.0f, 10.0f, 10.0f)), model_id_wall);

    const uint16_t test_voxel_model = context.load_voxel_model("test", scene);
    scene.add_voxel_object(glm::rotate(glm::scale(glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.0f, 5.0f)), glm::vec3(2.0f, 2.0f, 2.0f)), 1.0f, glm::vec3(0.0f, 0.2f, 0.8f)), test_voxel_model);
    
    context.allocate_vulkan_objects_for_scene(scene);
    context.build_bottom_level_acceleration_structure_for_model(model_id_dragon, scene);
    context.build_bottom_level_acceleration_structure_for_model(model_id_red_dragon, scene);
    context.build_bottom_level_acceleration_structure_for_model(model_id_blue_dragon, scene);
    context.build_bottom_level_acceleration_structure_for_model(model_id_pico, scene);
    context.build_bottom_level_acceleration_structure_for_model(model_id_floor, scene);
    context.build_bottom_level_acceleration_structure_for_model(model_id_wall, scene);
    context.build_bottom_level_acceleration_structure_for_voxel_model(test_voxel_model, scene);
    context.build_bottom_level_acceleration_structure_for_lights(scene);
    context.build_top_level_acceleration_structure_for_scene(scene);
    context.update_descriptors_tlas(scene);
    context.update_descriptors_ray_trace_objects(scene);
    context.update_descriptors_motion_vector_texture();
    context.update_descriptors_taa_images();
    context.update_descriptors_palettes(scene);
    context.update_descriptors_lights(scene);
    context.update_descriptors_perspective();
    
    //const float aspect_ratio = (float) context.swapchain_extent.width / (float) context.swapchain_extent.height;
    context.camera_position = glm::vec3(4.0f, 3.0f, 8.0f);

    auto system_time = std::chrono::system_clock::now();
    double elapsed_time_subsecond = 0.0;
    uint32_t num_frames_subsecond = 0;
    double elapsed_time = 0.0;
    context.camera_theta = 3.0 * M_PI / 4.0;
    context.camera_phi = 5.0 * M_PI / 4.0;
    const double sensitivity = 100.0, move_speed = 5.0;
    FrameMark;
    while (context.active) {
	const auto current_time = std::chrono::system_clock::now();
	const std::chrono::duration<double> dt_chrono = current_time - system_time;
	const double dt = dt_chrono.count();
	system_time = current_time;
	elapsed_time += dt;
	elapsed_time_subsecond += dt;
	++num_frames_subsecond;
	
	context.last_frame_view_dir = context.view_dir;
	context.last_frame_camera_position = context.camera_position;
	context.last_frame_camera_matrix = context.camera_matrix;
	context.view_dir = glm::vec3(sin(context.camera_theta) * cos(context.camera_phi), sin(context.camera_theta) * sin(context.camera_phi), cos(context.camera_theta));
	context.camera_matrix = glm::lookAt(context.camera_position, context.camera_position + context.view_dir, glm::vec3(0.0f, 0.0f, 1.0f));
	context.push_constants.current_frame = context.current_frame;
	context.push_constants.alpha_temporal = context.current_frame ? context.imgui_data.alpha_temporal : 0.0f;
	context.push_constants.alpha_taa = context.current_frame ? context.imgui_data.alpha_taa : 0.0f;
	context.push_constants.sigma_normal = context.imgui_data.sigma_normal;
	context.push_constants.sigma_position = context.imgui_data.sigma_position;
	context.push_constants.sigma_luminance = context.imgui_data.sigma_luminance;
	context.push_constants.num_filter_iters = context.imgui_data.atrous_filter_iters + 1;
	context.push_constants.temporal = context.imgui_data.temporal_filter;
	context.push_constants.taa = context.imgui_data.taa;
	if (!context.is_using_imgui()) {
	    const double mouse_dx = context.mouse_x - context.last_mouse_x;
	    const double mouse_dy = context.mouse_y - context.last_mouse_y;
	    if (context.pressed_buttons[GLFW_MOUSE_BUTTON_LEFT] == GLFW_PRESS) {
		context.camera_phi -= mouse_dx / sensitivity;
		context.camera_theta += mouse_dy / sensitivity;
		context.camera_theta = glm::clamp(context.camera_theta, 0.01, M_PI - 0.01);
		if (context.camera_phi < 0.0) context.camera_phi += 2.0 * M_PI;
		if (context.camera_phi >= 2.0 * M_PI) context.camera_phi -= 2.0 * M_PI;
	    }
	    if (context.pressed_keys[GLFW_KEY_W] == GLFW_PRESS) {
		context.camera_position += (float) (dt * move_speed) * glm::vec3(cos(context.camera_phi), sin(context.camera_phi), 0.0);
	    }
	    if (context.pressed_keys[GLFW_KEY_A] == GLFW_PRESS) {
		context.camera_position += (float) (dt * move_speed) * glm::vec3(-sin(context.camera_phi), cos(context.camera_phi), 0.0);
	    }
	    if (context.pressed_keys[GLFW_KEY_S] == GLFW_PRESS) {
		context.camera_position += (float) (dt * move_speed) * glm::vec3(-cos(context.camera_phi), -sin(context.camera_phi), 0.0);
	    }
	    if (context.pressed_keys[GLFW_KEY_D] == GLFW_PRESS) {
		context.camera_position += (float) (dt * move_speed) * glm::vec3(sin(context.camera_phi), -cos(context.camera_phi), 0.0);
	    }
	    if (context.pressed_keys[GLFW_KEY_LEFT_SHIFT] == GLFW_PRESS) {
		context.camera_position += (float) (dt * move_speed) * glm::vec3(0.0, 0.0, -1.0);
	    }
	    if (context.pressed_keys[GLFW_KEY_SPACE] == GLFW_PRESS) {
		context.camera_position += (float) (dt * move_speed) * glm::vec3(0.0, 0.0, 1.0);
	    }
	}
	if (!context.current_frame) {
	    context.last_frame_view_dir = context.view_dir;
	    context.last_frame_camera_position = context.camera_position;
	    context.last_frame_camera_matrix = context.camera_matrix;
	}
	context.last_pressed_keys = context.pressed_keys;

	/*scene.transforms[model_id_dragon][0] = glm::scale(glm::rotate(glm::translate(glm::mat4(1), glm::vec3(-2.5f, 0.0f, 0.0f)), (float) elapsed_time, glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(0.05f, 0.05f, 0.05f));
	
	context.ringbuffer_copy_scene_instances_into_buffer(scene);
	context.ringbuffer_copy_scene_lights_into_buffer(scene);*/
	context.ringbuffer_copy_projection_matrices_into_buffer();
	
	context.render();
	
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
	FrameMark;
    }

    vkDeviceWaitIdle(context.device);
    context.cleanup_vulkan_objects_for_scene(scene);
    context.cleanup();
    return 0;
}
