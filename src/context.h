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

#include <unordered_map>
#include <numeric>
#include <cstring>
#include <vector>
#include <array>
#include <tuple>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "scene.h"
#include "util.h"

static constexpr uint32_t FRAMES_IN_FLIGHT = 2;

struct SwapchainSupport {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

struct RenderContext {
    GLFWwindow *window;
    bool active = true, resized = false;
    uint32_t current_frame = 0;
    double elapsed_time = 0.0f;

    glm::mat4 perspective_matrix;
    glm::mat4 camera_matrix;
    glm::mat4 perspective_camera_matrix;
    
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue queue;

    VkSwapchainKHR swapchain;
    VkFormat swapchain_format;
    VkExtent2D swapchain_extent;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    std::vector<VkFramebuffer> swapchain_framebuffers;

    std::unordered_map<std::string, VkShaderModule> shader_modules;
    VkPipelineLayout raster_pipeline_layout;
    VkRenderPass raster_render_pass;
    VkPipeline raster_pipeline;

    VkCommandPool command_pool;
    std::array<VkCommandBuffer, FRAMES_IN_FLIGHT> raster_command_buffers;
    std::array<VkSemaphore, FRAMES_IN_FLIGHT> image_available_semaphores;
    std::array<VkSemaphore, FRAMES_IN_FLIGHT> render_finished_semaphores;
    std::array<VkFence, FRAMES_IN_FLIGHT> in_flight_fences;

    VmaAllocator allocator;

    std::array<bool, GLFW_KEY_LAST + 1> pressed_keys;

    auto init() noexcept -> void;
    auto render(double dt, const Scene &scene) noexcept -> void;
    auto cleanup() noexcept -> void;

    auto create_instance() noexcept -> void;
    auto create_surface() noexcept -> void;
    auto create_physical_device() noexcept -> void;
    auto create_device() noexcept -> void;
    auto create_allocator() noexcept -> void;
    auto create_swapchain() noexcept -> void;
    auto create_shaders() noexcept -> void;
    auto create_raster_pipeline() noexcept -> void;
    auto create_framebuffers() noexcept -> void;
    auto create_command_pool() noexcept -> void;
    auto create_command_buffers() noexcept -> void;
    auto create_sync_objects() noexcept -> void;

    auto cleanup_instance() noexcept -> void;
    auto cleanup_surface() noexcept -> void;
    auto cleanup_device() noexcept -> void;
    auto cleanup_allocator() noexcept -> void;
    auto cleanup_swapchain() noexcept -> void;
    auto cleanup_shaders() noexcept -> void;
    auto cleanup_raster_pipeline() noexcept -> void;
    auto cleanup_framebuffers() noexcept -> void;
    auto cleanup_command_pool() noexcept -> void;
    auto cleanup_sync_objects() noexcept -> void;

    auto physical_check_queue_family(VkPhysicalDevice physical_device, VkQueueFlagBits bits) noexcept -> uint32_t;
    auto physical_check_extensions(VkPhysicalDevice physical_device) noexcept -> int32_t;
    auto physical_check_swapchain_support(VkPhysicalDevice physical_device) noexcept -> SwapchainSupport;
    auto physical_check_features_support(VkPhysicalDevice physical_device) noexcept -> int32_t;
    auto physical_score(const VkPhysicalDevice physical) noexcept -> int32_t;

    auto choose_swapchain_options(const SwapchainSupport &support) noexcept -> std::tuple<VkSurfaceFormatKHR, VkPresentModeKHR, VkExtent2D>;

    auto create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_flags, VmaAllocationCreateFlags vma_flags = 0) noexcept -> Buffer;
    auto cleanup_buffer(Buffer buffer) noexcept -> void;
    auto create_image(VkImageCreateFlags flags, VkFormat format, VkExtent3D extent, uint32_t mipLevels, uint32_t arrayLevels, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_flags, VmaAllocationCreateFlags vma_flags = 0) noexcept -> Image;
    auto cleanup_image(Image image) noexcept -> void;
    auto create_image_view(VkImage image, VkImageViewType type, VkFormat format, VkImageSubresourceRange subresource_range) noexcept -> VkImageView;
    auto cleanup_image_view(VkImageView view) noexcept -> void;

    auto record_raster_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index, const Scene &scene) noexcept -> void;

    auto create_semaphore() noexcept -> VkSemaphore;
    auto create_fence() noexcept -> VkFence;

    auto recreate_swapchain() noexcept -> void;

    auto allocate_vulkan_objects_for_scene(Scene &scene) noexcept -> void;
    auto cleanup_vulkan_objects_for_scene(Scene &scene) noexcept -> void;
    auto inefficient_copy_scene_data_into_buffers(Scene &scene, std::size_t vertex_size, std::size_t index_size, std::size_t instance_size, std::size_t indirect_draw_size) noexcept -> void;
    auto inefficient_copy_buffers(Buffer dst, Buffer src, VkBufferCopy copy_region) noexcept -> void;
};

#endif
