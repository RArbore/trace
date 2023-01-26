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

#include <filesystem>
#include <fstream>

#include "context.h"

static constexpr std::string_view DEFAULT_SHADER_PATH = "build";

auto RenderContext::create_shaders() noexcept -> void {
    ASSERT(std::filesystem::exists(DEFAULT_SHADER_PATH) && std::filesystem::is_directory(DEFAULT_SHADER_PATH), "");
    for (const auto& entry : std::filesystem::directory_iterator(DEFAULT_SHADER_PATH)) {
	const auto filename = entry.path().filename();
	if (filename.extension() == ".spv") {
	    std::ifstream fstream(entry.path(), std::ios::in | std::ios::binary);
	    const auto size = std::filesystem::file_size(entry.path());
	    std::string result(size, '\0');
	    fstream.read(result.data(), size);

	    VkShaderModuleCreateInfo create_info {};
	    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	    create_info.codeSize = result.size();
	    create_info.pCode = (uint32_t*) result.data();

	    ASSERT(vkCreateShaderModule(device, &create_info, NULL, &shader_modules[filename.stem()]), "Unable to create shader module.");
	    std::cout << "INFO: Loaded shader " << filename << ".\n";
	}
    }
}

auto RenderContext::cleanup_shaders() noexcept -> void {
    for (auto [_, module] : shader_modules) {
	vkDestroyShaderModule(device, module, NULL);
    }
}

auto RenderContext::create_raster_pipeline() noexcept -> void {
    VkShaderModule vertex_shader = shader_modules["pbr_vertex"];
    VkShaderModule fragment_shader = shader_modules["pbr_fragment"];

    VkPipelineShaderStageCreateInfo vertex_shader_stage_create_info {};
    vertex_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_create_info.module = vertex_shader;
    vertex_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_create_info {};
    fragment_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_create_info.module = fragment_shader;
    fragment_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = {vertex_shader_stage_create_info, fragment_shader_stage_create_info};

    auto binding_descriptions = RasterScene::binding_descriptions();
    auto attribute_descriptions = RasterScene::attribute_descriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info {};
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.vertexBindingDescriptionCount = binding_descriptions.size();
    vertex_input_create_info.pVertexBindingDescriptions = binding_descriptions.data();
    vertex_input_create_info.vertexAttributeDescriptionCount = attribute_descriptions.size();
    vertex_input_create_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info {};
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state_create_info {};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info {};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.lineWidth = 1.0f;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info {};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling_state_create_info {};
    multisampling_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_state_create_info.sampleShadingEnable = VK_FALSE;
    multisampling_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state {};
    color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state.blendEnable = VK_TRUE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending_state_create_info {};
    color_blending_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_state_create_info.logicOpEnable = VK_FALSE;
    color_blending_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blending_state_create_info.attachmentCount = 1;
    color_blending_state_create_info.pAttachments = &color_blend_attachment_state;

    VkPushConstantRange push_constant_range {};
    push_constant_range.offset = 0;
    push_constant_range.size = 128;
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDynamicState pipeline_dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info {};
    pipeline_dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipeline_dynamic_state_create_info.dynamicStateCount = 2;
    pipeline_dynamic_state_create_info.pDynamicStates = pipeline_dynamic_states;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &raster_descriptor_set_layout;
    ASSERT(vkCreatePipelineLayout(device, &pipeline_layout_create_info, NULL, &raster_pipeline_layout), "Unable to create raster pipeline layout.");

    VkAttachmentDescription color_attachment {};
    color_attachment.format = swapchain_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_reference {};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth_attachment {};
    depth_attachment.format = VK_FORMAT_D32_SFLOAT;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference {};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;
    subpass.pDepthStencilAttachment = &depth_attachment_reference;

    VkAttachmentDescription attachments[] = {color_attachment, depth_attachment};
    VkSubpassDescription subpasses[] = {subpass};

    VkSubpassDependency subpass_dependency {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo render_pass_create_info {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 2;
    render_pass_create_info.pAttachments = attachments;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = subpasses;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &subpass_dependency;

    ASSERT(vkCreateRenderPass(device, &render_pass_create_info, NULL, &raster_render_pass), "Unable to create raster render pass.");

    VkGraphicsPipelineCreateInfo raster_pipeline_create_info {};
    raster_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    raster_pipeline_create_info.stageCount = 2;
    raster_pipeline_create_info.pStages = shader_stage_create_infos;
    raster_pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    raster_pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    raster_pipeline_create_info.pViewportState = &viewport_state_create_info;
    raster_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    raster_pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    raster_pipeline_create_info.pMultisampleState = &multisampling_state_create_info;
    raster_pipeline_create_info.pColorBlendState = &color_blending_state_create_info;
    raster_pipeline_create_info.pDynamicState = &pipeline_dynamic_state_create_info;
    raster_pipeline_create_info.layout = raster_pipeline_layout;
    raster_pipeline_create_info.renderPass = raster_render_pass;
    raster_pipeline_create_info.subpass = 0;
    raster_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

    ASSERT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &raster_pipeline_create_info, NULL, &raster_pipeline), "Unable to create raster pipeline.");
}

auto RenderContext::cleanup_raster_pipeline() noexcept -> void {
    vkDestroyPipeline(device, raster_pipeline, NULL);
    vkDestroyRenderPass(device, raster_render_pass, NULL);
    vkDestroyPipelineLayout(device, raster_pipeline_layout, NULL);
}

auto RenderContext::create_ray_trace_pipeline() noexcept -> void {
    VkShaderModule rgen_shader = shader_modules["pbr_rgen"];
    VkShaderModule rmiss_shader = shader_modules["pbr_rmiss"];
    VkShaderModule rchit_shader = shader_modules["pbr_rchit"];

    VkPipelineShaderStageCreateInfo rgen_shader_stage_create_info {};
    rgen_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    rgen_shader_stage_create_info.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    rgen_shader_stage_create_info.module = rgen_shader;
    rgen_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo rmiss_shader_stage_create_info {};
    rmiss_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    rmiss_shader_stage_create_info.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    rmiss_shader_stage_create_info.module = rmiss_shader;
    rmiss_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo rchit_shader_stage_create_info {};
    rchit_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    rchit_shader_stage_create_info.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    rchit_shader_stage_create_info.module = rchit_shader;
    rchit_shader_stage_create_info.pName = "main";

    VkRayTracingShaderGroupCreateInfoKHR shader_group_create_info {};
    shader_group_create_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    shader_group_create_info.anyHitShader = VK_SHADER_UNUSED_KHR;
    shader_group_create_info.closestHitShader = VK_SHADER_UNUSED_KHR;
    shader_group_create_info.generalShader = VK_SHADER_UNUSED_KHR;
    shader_group_create_info.intersectionShader = VK_SHADER_UNUSED_KHR;
    
    shader_group_create_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    shader_group_create_info.generalShader = 0;
    ray_trace_shader_groups.push_back(shader_group_create_info);
    
    shader_group_create_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    shader_group_create_info.generalShader = 1;
    ray_trace_shader_groups.push_back(shader_group_create_info);

    shader_group_create_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    shader_group_create_info.generalShader = VK_SHADER_UNUSED_KHR;
    shader_group_create_info.closestHitShader = 2;
    ray_trace_shader_groups.push_back(shader_group_create_info);
}

auto RenderContext::cleanup_ray_trace_pipeline() noexcept -> void {

}

auto RenderContext::create_framebuffers() noexcept -> void {
    swapchain_framebuffers.resize(swapchain_images.size());

    for (std::size_t i = 0; i < swapchain_framebuffers.size(); ++i) {
	VkImageView attachments[] = {swapchain_image_views[i], depth_image_view};

	VkFramebufferCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	create_info.renderPass = raster_render_pass;
	create_info.attachmentCount = 2;
	create_info.pAttachments = attachments;
	create_info.width = swapchain_extent.width;
	create_info.height = swapchain_extent.height;
	create_info.layers = 1;

	ASSERT(vkCreateFramebuffer(device, &create_info, NULL, &swapchain_framebuffers[i]), "Unable to create framebuffer.");
    }
}

auto RenderContext::cleanup_framebuffers() noexcept -> void {
    for (auto framebuffer : swapchain_framebuffers) {
	vkDestroyFramebuffer(device, framebuffer, NULL);
    }
}
