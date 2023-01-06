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
    context->resized = true;
}

static auto glfw_key_callback(GLFWwindow* window, int key, __attribute__((unused)) int scancode, int action, __attribute__((unused)) int mods) noexcept -> void {
    ASSERT(key >= 0 && key < GLFW_KEY_LAST, "Pressed unknown key.");
    RenderContext *context = (RenderContext*) glfwGetWindowUserPointer(window);
    context->pressed_keys[key] = action != GLFW_RELEASE;
}

auto RenderContext::init() noexcept -> void {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "trace", NULL, NULL);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_resize_callback);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    create_instance();
    create_surface();
    create_physical_device();
    create_device();
    create_swapchain();
    create_shaders();
}

auto RenderContext::render() noexcept -> void {
    glfwPollEvents();
    if (pressed_keys[GLFW_KEY_ESCAPE] || glfwWindowShouldClose(window)) {
	active = false;
    }
    resized = false;
}

auto RenderContext::cleanup() noexcept -> void {
    vkDeviceWaitIdle(device);

    cleanup_shaders();
    cleanup_swapchain();
    cleanup_device();
    cleanup_surface();
    cleanup_instance();
    glfwDestroyWindow(window);
    glfwTerminate();
}
