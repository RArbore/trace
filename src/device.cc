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

static const char* validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

static const char* device_extensions[] = {
    "VK_KHR_swapchain",
};

auto RenderContext::create_instance() noexcept -> void {
    VkApplicationInfo app_info {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "trace";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Custom";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    
    uint32_t glfw_extension_count = 0;
    const char ** const glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;
    
#ifndef RELEASE
    create_info.enabledLayerCount = sizeof(validation_layers) / sizeof(validation_layers[0]);
    create_info.ppEnabledLayerNames = validation_layers;
#else
    create_info.enabledLayerCount = 0;
#endif
    
    ASSERT(vkCreateInstance(&create_info, NULL, &instance), "Couldn't create Vulkan instance.");
}

auto RenderContext::create_surface() noexcept -> void {
    ASSERT(glfwCreateWindowSurface(instance, window, NULL, &surface), "Couldn't create GLFW window surface.");
}

auto RenderContext::cleanup_instance() noexcept -> void {
    vkDestroyInstance(instance, NULL);
}

auto RenderContext::cleanup_surface() noexcept -> void {
    vkDestroySurfaceKHR(instance, surface, NULL);
}
