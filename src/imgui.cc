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

#include <sstream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "context.h"

auto RenderContext::init_imgui() noexcept -> void {
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo vulkan_init_info = {};
    vulkan_init_info.Instance = instance;
    vulkan_init_info.PhysicalDevice = physical_device;
    vulkan_init_info.Device = device;
    vulkan_init_info.Queue = queue;
    vulkan_init_info.DescriptorPool = imgui_descriptor_pool;
    vulkan_init_info.MinImageCount = 3;
    vulkan_init_info.ImageCount = 3;
    vulkan_init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    
    ImGui_ImplVulkan_Init(&vulkan_init_info, raster_render_pass);
    ImGui::StyleColorsDark();

    VkCommandBufferAllocateInfo allocate_info {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    
    VkCommandBuffer imgui_font_command_buffer;
    ASSERT(vkAllocateCommandBuffers(device, &allocate_info, &imgui_font_command_buffer), "Unable to create command buffers.");

    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(imgui_font_command_buffer, &command_buffer_begin_info);
    ImGui_ImplVulkan_CreateFontsTexture(imgui_font_command_buffer);
    vkEndCommandBuffer(imgui_font_command_buffer);

    VkSubmitInfo submit_info {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &imgui_font_command_buffer;
    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

auto RenderContext::cleanup_imgui() noexcept -> void {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

auto RenderContext::recreate_imgui() noexcept -> void {
    ImGui_ImplVulkan_SetMinImageCount((uint32_t) swapchain_images.size());
}

auto RenderContext::render_imgui(Scene &scene) noexcept -> void {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::InputText("Name of model", imgui_data.model_name.data(), imgui_data.model_name.size());
    ImGui::InputFloat("X position of model", &imgui_data.model_position[0]);
    ImGui::InputFloat("Y position of model", &imgui_data.model_position[1]);
    ImGui::InputFloat("Z position of model", &imgui_data.model_position[2]);
    if (ImGui::Button("Load model")) {
	uint16_t model_id = load_model(imgui_data.model_name.data(), scene);
	scene.add_object(glm::translate(glm::mat4(1), glm::vec3(imgui_data.model_position[0], imgui_data.model_position[1], imgui_data.model_position[2])), model_id);
	update_vulkan_objects_for_scene(scene);
    }
    ImGui::InputFloat("Intensity of light", &imgui_data.light_intensity);
    ImGui::InputFloat("X position of light", &imgui_data.light_position[0]);
    ImGui::InputFloat("Y position of light", &imgui_data.light_position[1]);
    ImGui::InputFloat("Z position of light", &imgui_data.light_position[2]);
    if (ImGui::Button("Add light")) {
	scene.add_light({imgui_data.light_position[0], imgui_data.light_position[1], imgui_data.light_position[2], imgui_data.light_intensity});
	update_vulkan_objects_for_scene(scene);
	update_descriptors_lights(scene);
    }
    std::ostringstream fps_label;
    fps_label << "FPS: " << imgui_data.last_fpss.back();
    ImGui::PlotLines(fps_label.str().c_str(), imgui_data.last_fpss.data(), (int32_t) imgui_data.last_fpss.size());
    std::ostringstream heap_label;
    heap_label << "HEAP: " << imgui_data.last_heaps.back();
    ImGui::PlotLines(heap_label.str().c_str(), imgui_data.last_heaps.data(), (int32_t) imgui_data.last_heaps.size());
    
    ImGui::Render();
}

auto RenderContext::render_draw_data_wrapper_imgui(VkCommandBuffer command_buffer) noexcept -> void {
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
}

auto RenderContext::is_using_imgui() noexcept -> bool {
    return ImGui::GetIO().WantCaptureMouse;
}
