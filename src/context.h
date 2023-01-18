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

#include <numeric>
#include <cstring>
#include <vector>
#include <array>
#include <tuple>
#include <map>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "scene.h"
#include "util.h"

#define VKFN_MEMBER(fn)				\
    PFN_ ## fn fn

#define VKFN_INIT(fn)				\
    fn = (PFN_ ## fn) vkGetDeviceProcAddr(device, #fn)

static constexpr uint32_t FRAMES_IN_FLIGHT = 2;

struct SwapchainSupport {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

struct ImGuiData {
    std::array<char, 100> model_name;
    std::array<float, 3> model_position;
    float light_intensity;
    std::array<float, 3> light_position;
};

struct RenderContext {
    GLFWwindow *window;
    bool active = true, resized = false;
    uint32_t current_frame = 0;

    glm::mat4 perspective_matrix;
    glm::mat4 camera_matrix;
    glm::mat4 perspective_camera_matrix;
    glm::vec3 camera_position;
    
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

    std::map<std::string, VkShaderModule> shader_modules;
    VkPipelineLayout raster_pipeline_layout;
    VkRenderPass raster_render_pass;
    VkPipeline raster_pipeline;

    Image depth_image;
    VkImageView depth_image_view;
    RingBuffer main_ring_buffer;

    VkCommandPool command_pool;
    std::array<VkCommandBuffer, FRAMES_IN_FLIGHT> raster_command_buffers;
    std::array<VkSemaphore, FRAMES_IN_FLIGHT> image_available_semaphores;
    std::array<VkSemaphore, FRAMES_IN_FLIGHT> render_finished_semaphores;
    std::array<VkFence, FRAMES_IN_FLIGHT> in_flight_fences;
    std::vector<VkSemaphore> ring_buffer_semaphore_scratchpad;
    std::vector<VkPipelineStageFlags> ring_buffer_wait_stages_scratchpad;

    VkSampler sampler;
    VkDescriptorPool descriptor_pool, imgui_descriptor_pool;
    VkDescriptorSetLayout raster_descriptor_set_layout;
    std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> raster_descriptor_sets;
    std::multimap<uint32_t, std::tuple<VkWriteDescriptorSet, VkDescriptorBufferInfo, VkDescriptorImageInfo>> raster_descriptor_set_writes;

    VmaAllocator allocator;
    std::vector<std::pair<Buffer, std::size_t>> buffer_cleanup_queue;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_properties;
    VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_properties;

    ImGuiData imgui_data;

    double mouse_x;
    double mouse_y;
    double last_mouse_x;
    double last_mouse_y;
    std::array<float, 50> last_fpss;
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> pressed_buttons;
    std::array<bool, GLFW_KEY_LAST + 1> pressed_keys;

    auto init() noexcept -> void;
    auto render(RasterScene &scene) noexcept -> void;
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
    auto create_sampler() noexcept -> void;
    auto create_descriptor_pool() noexcept -> void;
    auto create_descriptor_set_layout() noexcept -> void;
    auto create_descriptor_sets() noexcept -> void;
    auto create_command_pool() noexcept -> void;
    auto create_depth_resources() noexcept -> void;
    auto create_command_buffers() noexcept -> void;
    auto create_sync_objects() noexcept -> void;
    auto create_ringbuffer() noexcept -> RingBuffer;

    auto cleanup_instance() noexcept -> void;
    auto cleanup_surface() noexcept -> void;
    auto cleanup_device() noexcept -> void;
    auto cleanup_allocator() noexcept -> void;
    auto cleanup_swapchain() noexcept -> void;
    auto cleanup_shaders() noexcept -> void;
    auto cleanup_raster_pipeline() noexcept -> void;
    auto cleanup_framebuffers() noexcept -> void;
    auto cleanup_sampler() noexcept -> void;
    auto cleanup_descriptor_pool() noexcept -> void;
    auto cleanup_descriptor_set_layout() noexcept -> void;
    auto cleanup_command_pool() noexcept -> void;
    auto cleanup_depth_resources() noexcept -> void;
    auto cleanup_sync_objects() noexcept -> void;
    auto cleanup_ringbuffer(RingBuffer &ring_buffer) noexcept -> void;

    auto physical_check_queue_family(VkPhysicalDevice physical_device, VkQueueFlagBits bits) noexcept -> uint32_t;
    auto physical_check_extensions(VkPhysicalDevice physical_device) noexcept -> int32_t;
    auto physical_check_swapchain_support(VkPhysicalDevice physical_device) noexcept -> SwapchainSupport;
    auto physical_check_features_support(VkPhysicalDevice physical_device) noexcept -> int32_t;
    auto physical_score(const VkPhysicalDevice physical) noexcept -> int32_t;

    auto choose_swapchain_options(const SwapchainSupport &support) noexcept -> std::tuple<VkSurfaceFormatKHR, VkPresentModeKHR, VkExtent2D>;

    auto create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_flags = 0, VmaAllocationCreateFlags vma_flags = 0) noexcept -> Buffer;
    auto create_buffer_with_alignment(VkDeviceSize size, VkDeviceSize alignment, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_flags = 0, VmaAllocationCreateFlags vma_flags = 0) noexcept -> Buffer;
    auto cleanup_buffer(Buffer buffer) noexcept -> void;
    auto future_cleanup_buffer(Buffer buffer) noexcept -> void;
    auto create_image(VkImageCreateFlags flags, VkFormat format, VkExtent2D extent, uint32_t mip_levels, uint32_t array_layers, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_flags, VmaAllocationCreateFlags vma_flags = 0) noexcept -> Image;
    auto cleanup_image(Image image) noexcept -> void;
    auto create_image_view(VkImage image, VkFormat format, VkImageSubresourceRange subresource_range) noexcept -> VkImageView;
    auto cleanup_image_view(VkImageView view) noexcept -> void;

    auto record_raster_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index, const RasterScene &scene) noexcept -> void;

    auto create_semaphore() noexcept -> VkSemaphore;
    auto create_fence() noexcept -> VkFence;

    auto recreate_swapchain() noexcept -> void;

    auto allocate_vulkan_objects_for_scene(RasterScene &scene) noexcept -> void;
    auto cleanup_vulkan_objects_for_scene(RasterScene &scene) noexcept -> void;
    auto update_vulkan_objects_for_scene(RasterScene &scene) noexcept -> void;
    auto ringbuffer_copy_scene_vertices_into_buffer(RasterScene &scene) noexcept -> void;
    auto ringbuffer_copy_scene_indices_into_buffer(RasterScene &scene) noexcept -> void;
    auto ringbuffer_copy_scene_instances_into_buffer(RasterScene &scene) noexcept -> void;
    auto ringbuffer_copy_scene_indirect_draw_into_buffer(RasterScene &scene) noexcept -> void;
    auto ringbuffer_copy_scene_lights_into_buffer(RasterScene &scene) noexcept -> void;

    auto ringbuffer_claim_buffer(RingBuffer &ring_buffer, std::size_t size) noexcept -> void *;
    auto ringbuffer_submit_buffer(RingBuffer &ring_buffer, Buffer &dst) noexcept -> void;
    auto ringbuffer_submit_buffer(RingBuffer &ring_buffer, Image dst) noexcept -> void;

    auto load_model(std::string_view model_name, RasterScene &scene) noexcept -> uint16_t;
    auto load_obj_model(std::string_view obj_filepath) noexcept -> Model;
    auto load_texture(std::string_view texture_filepath, bool srgb) noexcept -> std::pair<Image, VkImageView>;

    auto update_descriptors_textures(const RasterScene &scene, uint32_t update_texture) noexcept -> void;
    auto update_descriptors_lights(const RasterScene &scene) noexcept -> void;

    auto get_device_address(const Buffer &buffer) noexcept -> VkDeviceAddress;
    auto get_device_address(const VkAccelerationStructureKHR &acceleration_structure) noexcept -> VkDeviceAddress;
    auto build_acceleration_structure_for_scene(RasterScene &scene) noexcept -> void;

    auto init_imgui() noexcept -> void;
    auto cleanup_imgui() noexcept -> void;
    auto recreate_imgui() noexcept -> void;
    auto render_imgui(RasterScene &scene) noexcept -> void;
    auto render_draw_data_wrapper_imgui(VkCommandBuffer command_buffer) noexcept -> void;
    auto is_using_imgui() noexcept -> bool;

    VkCommandBuffer inefficient_one_time_command_buffer = VK_NULL_HANDLE;
    auto inefficient_run_commands(auto F) noexcept -> void {
	if (inefficient_one_time_command_buffer == VK_NULL_HANDLE) {
	    VkCommandBufferAllocateInfo allocate_info {};
	    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	    allocate_info.commandPool = command_pool;
	    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	    allocate_info.commandBufferCount = 1;
	    
	    ASSERT(vkAllocateCommandBuffers(device, &allocate_info, &inefficient_one_time_command_buffer), "Unable to create one-time command buffer.");
	}

	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	
	ASSERT(vkBeginCommandBuffer(inefficient_one_time_command_buffer, &begin_info), "Unable to begin recording one-time command buffer.");
	F(inefficient_one_time_command_buffer);
	ASSERT(vkEndCommandBuffer(inefficient_one_time_command_buffer), "Something went wrong recording into one-time rcommand buffer.");

	VkSubmitInfo submit_info {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &inefficient_one_time_command_buffer;

	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);
    }
    
    VKFN_MEMBER(vkGetAccelerationStructureBuildSizesKHR);
    VKFN_MEMBER(vkCreateAccelerationStructureKHR);
    VKFN_MEMBER(vkCmdBuildAccelerationStructuresKHR);
    VKFN_MEMBER(vkGetAccelerationStructureDeviceAddressKHR);
    
    auto init_vk_funcs() noexcept -> void {
	VKFN_INIT(vkGetAccelerationStructureBuildSizesKHR);
	VKFN_INIT(vkCreateAccelerationStructureKHR);
	VKFN_INIT(vkCmdBuildAccelerationStructuresKHR);
	VKFN_INIT(vkGetAccelerationStructureDeviceAddressKHR);
    }
};

#endif
