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

#ifndef CONTEXT_H
#define CONTEXT_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "util.h"

struct RenderContext {
    GLFWwindow *window;
    int width, height;
    bool active = true;

    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;

    bool pressed_keys[GLFW_KEY_LAST + 1];

    auto init() noexcept -> void;
    auto render() noexcept -> void;
    auto cleanup() noexcept -> void;

    auto create_instance() noexcept -> void;
    auto create_surface() noexcept -> void;
    auto create_physical_device() noexcept -> void;

    auto cleanup_instance() noexcept -> void;
    auto cleanup_surface() noexcept -> void;
    auto cleanup_physical_device() noexcept -> void;
};

#endif
