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

#include "Tracy.hpp"

#include "context.h"

#ifndef RELEASE
static const char* validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};
#endif

static const char* device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
};

auto RenderContext::create_instance() noexcept -> void {
    ZoneScoped;
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
    ZoneScoped;
    ASSERT(glfwCreateWindowSurface(instance, window, NULL, &surface), "Couldn't create GLFW window surface.");
}

auto RenderContext::physical_check_queue_family(VkPhysicalDevice physical, VkQueueFlagBits bits) noexcept -> uint32_t {
    ZoneScoped;
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queue_family_count, NULL);
    ASSERT(queue_family_count > 0, "No queue families.");

    std::vector<VkQueueFamilyProperties> possible(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queue_family_count, &possible[0]);

    for (uint32_t queue_family_index = 0; queue_family_index < queue_family_count; ++queue_family_index) {
	if ((possible[queue_family_index].queueFlags & bits) == bits) {
	    VkBool32 present_support = VK_FALSE;
	    vkGetPhysicalDeviceSurfaceSupportKHR(physical, queue_family_index, surface, &present_support);
	    if (present_support == VK_TRUE) {
		return queue_family_index;
	    }
	}
    }
    
    return 0xFFFFFFFF;
}

auto RenderContext::physical_check_extensions(VkPhysicalDevice physical) noexcept -> int32_t {
    ZoneScoped;
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(physical, NULL, &extension_count, NULL);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(physical, NULL, &extension_count, &available_extensions[0]);

    uint32_t required_extension_index;
    for (required_extension_index = 0; required_extension_index < sizeof(device_extensions) / sizeof(device_extensions[0]); ++required_extension_index) {
	uint32_t available_extension_index;
	for (available_extension_index = 0; available_extension_index < extension_count; ++available_extension_index) {
	    if (!strcmp(available_extensions[available_extension_index].extensionName, device_extensions[required_extension_index])) {
		break;
	    }
	}
	if (available_extension_index >= extension_count) {
	    break;
	}
    }
    if (required_extension_index < sizeof(device_extensions) / sizeof(device_extensions[0])) {
	return -1;
    }
    else {
	return 0;
    }
}

auto RenderContext::physical_check_swapchain_support(VkPhysicalDevice specific_physical_device) noexcept -> SwapchainSupport {
    ZoneScoped;
    uint32_t num_formats, num_present_modes;
    vkGetPhysicalDeviceSurfaceFormatsKHR(specific_physical_device, surface, &num_formats, NULL);
    vkGetPhysicalDeviceSurfacePresentModesKHR(specific_physical_device, surface, &num_present_modes, NULL);

    SwapchainSupport ss { {}, std::vector<VkSurfaceFormatKHR>(num_formats), std::vector<VkPresentModeKHR>(num_present_modes) };
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(specific_physical_device, surface, &ss.capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(specific_physical_device, surface, &num_formats, &ss.formats[0]);
    vkGetPhysicalDeviceSurfacePresentModesKHR(specific_physical_device, surface, &num_present_modes, &ss.present_modes[0]);

    return ss;
}

auto RenderContext::physical_check_features_support(VkPhysicalDevice physical) noexcept -> int32_t {
    ZoneScoped;
    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features {};
    buffer_device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    buffer_device_address_features.pNext = NULL;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_features {};
    acceleration_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    acceleration_features.pNext = &buffer_device_address_features;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_features {};
    ray_tracing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    ray_tracing_features.pNext = &acceleration_features;

    VkPhysicalDeviceVulkan11Features vulkan_11_features {};
    vulkan_11_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan_11_features.pNext = &ray_tracing_features;

    VkPhysicalDeviceDescriptorIndexingFeatures indexing_features {};
    indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    indexing_features.pNext = &vulkan_11_features;

    VkPhysicalDeviceFeatures2 device_features {};
    device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    device_features.features.samplerAnisotropy = VK_TRUE;
    device_features.pNext = &indexing_features;
    
    vkGetPhysicalDeviceFeatures2(physical, &device_features);
    
    if (indexing_features.descriptorBindingPartiallyBound &&
	indexing_features.runtimeDescriptorArray &&
	vulkan_11_features.shaderDrawParameters &&
	ray_tracing_features.rayTracingPipeline &&
	acceleration_features.accelerationStructure &&
	acceleration_features.descriptorBindingAccelerationStructureUpdateAfterBind &&
	buffer_device_address_features.bufferDeviceAddress
	) {
	return 0;
    }
    return -1;
}

auto RenderContext::physical_score(const VkPhysicalDevice physical) noexcept -> int32_t {
    ZoneScoped;
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical, &device_properties);

    int32_t device_type_score = 0;
    switch (device_properties.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
	device_type_score = 0;
	break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
	device_type_score = 1;
	break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
	device_type_score = 2;
	break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
	device_type_score = 3;
	break;
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
	device_type_score = 4;
	break;
    default:
	device_type_score = 0;
    }

    const uint32_t queue_check = physical_check_queue_family(physical, (VkQueueFlagBits) (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));
    if (queue_check == 0xFFFFFFFF) {
	return -1;
    }

    const int32_t extension_check = physical_check_extensions(physical);
    if (extension_check == -1) {
	return -1;
    }

    const SwapchainSupport support_check = physical_check_swapchain_support(physical);
    if (support_check.formats.size() == 0 || support_check.present_modes.size() == 0) {
	return -1;
    }

    const int32_t features_check = physical_check_features_support(physical);
    if (features_check == -1) {
	return -1;
    }

    return device_type_score;
}

auto RenderContext::create_physical_device() noexcept -> void {
    ZoneScoped;
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    ASSERT(device_count > 0, "No physical devices.");

    std::vector<VkPhysicalDevice> possible(device_count);
    ASSERT(vkEnumeratePhysicalDevices(instance, &device_count, &possible[0]), "Couldn't enumerate all physical devices.");

    int32_t best_physical = -1;
    int32_t best_score = 0;
    for (uint32_t new_physical = 0; new_physical < device_count; ++new_physical) {
	const int32_t new_score = physical_score(possible[new_physical]);
	if (new_score > best_score) {
	    best_score = new_score;
	    best_physical = new_physical;
	}
    }

    ASSERT(best_physical != -1, "No physical device is suitable.");
    physical_device = possible[best_physical];

    VkPhysicalDeviceProperties2 device_properties;
    device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    device_properties.pNext = &ray_tracing_properties;
    ray_tracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    ray_tracing_properties.pNext = &acceleration_structure_properties;
    acceleration_structure_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
    vkGetPhysicalDeviceProperties2(physical_device, &device_properties);
    std::cout << "INFO: Using device " << device_properties.properties.deviceName << ".\n";
}

auto RenderContext::create_device() noexcept -> void {
    ZoneScoped;
    const uint32_t queue_family = physical_check_queue_family(physical_device, (VkQueueFlagBits) (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));
    ASSERT(queue_family, "Could not find queue family.");

    const float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo queue_create_info {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features {};
    buffer_device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    buffer_device_address_features.bufferDeviceAddress = VK_TRUE; 
    buffer_device_address_features.pNext = NULL;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_features {};
    acceleration_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    acceleration_features.accelerationStructure = VK_TRUE;
    acceleration_features.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;
    acceleration_features.pNext = &buffer_device_address_features;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_features {};
    ray_tracing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    ray_tracing_features.rayTracingPipeline = VK_TRUE;
    ray_tracing_features.pNext = &acceleration_features;

    VkPhysicalDeviceVulkan11Features vulkan_11_features {};
    vulkan_11_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan_11_features.shaderDrawParameters = VK_TRUE;
    vulkan_11_features.pNext = &ray_tracing_features;

    VkPhysicalDeviceDescriptorIndexingFeatures indexing_features {};
    indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    indexing_features.descriptorBindingPartiallyBound = VK_TRUE;
    indexing_features.runtimeDescriptorArray = VK_TRUE;
    indexing_features.pNext = &vulkan_11_features;

    VkPhysicalDeviceFeatures2 device_features {};
    device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    device_features.features.samplerAnisotropy = VK_TRUE;
    device_features.pNext = &indexing_features;

    vkGetPhysicalDeviceFeatures2(physical_device, &device_features);

    VkDeviceCreateInfo device_create_info {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.pNext = &device_features;
    device_create_info.enabledExtensionCount = sizeof(device_extensions) / sizeof(device_extensions[0]);
    device_create_info.ppEnabledExtensionNames = device_extensions;

    ASSERT(vkCreateDevice(physical_device, &device_create_info, NULL, &device), "Couldn't create logical device.");
    vkGetDeviceQueue(device, queue_family, 0, &queue);
    init_vk_funcs();
}

auto RenderContext::cleanup_instance() noexcept -> void {
    ZoneScoped;
    vkDestroyInstance(instance, NULL);
}

auto RenderContext::cleanup_surface() noexcept -> void {
    ZoneScoped;
    vkDestroySurfaceKHR(instance, surface, NULL);
}

auto RenderContext::cleanup_device() noexcept -> void {
    ZoneScoped;
    vkDestroyDevice(device, NULL);
}
