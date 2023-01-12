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

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "context.h"

auto RenderContext::init_imgui() noexcept -> void {
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(window, false);

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
}

auto RenderContext::cleanup_imgui() noexcept -> void {
    ImGui_ImplVulkan_Shutdown();
}
