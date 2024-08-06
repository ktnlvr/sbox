#pragma once

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <vulkan/vulkan.h>

#include <fstream>
#include <vector>

#include "bootstrap.hpp"

#include "vertex.hpp"

namespace b::engine {

static Mesh* mesh;

struct FrameData {
  VkImage swapchain_image;
  VkImageView swapchain_image_view;
  VkFramebuffer framebuffer;
  VkCommandBuffer command_buffer;
  VkFence image_in_flight;
};

struct FrameInFlight {
  VkSemaphore available_semaphore, finished_semaphore;
  VkFence in_flight_fence;
};

VkShaderModule create_shader_module(BootstrapInfo &bootstrap,
                                    const char *filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  CHECK_REPORT_FMT(file, "Failed to open the file {}", filename);
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<char> buffer(size);
  CHECK(!file.read(buffer.data(), size).bad());

  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = size;
  create_info.pCode = (uint32_t *)buffer.data();

  VkShaderModule shader;
  CHECK_VK(bootstrap.dispatch.createShaderModule(&create_info, NULL, &shader));
  return shader;
}

struct RenderData {
  uint32_t MAX_FRAMES_IN_FLIGHT = 2;

  VkQueue graphics_queue;
  VkQueue present_queue;

  VkRenderPass render_pass;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;

  VkCommandPool command_pool;

  std::vector<FrameData> frames;
  std::vector<FrameInFlight> frames_in_flight;

  size_t current_frame = 0;

  VkDescriptorPool imgui_descriptor_pool;

  void init_queues(BootstrapInfo &bootstrap) {
    auto graphics_queue_ret =
        bootstrap.device.get_queue(vkb::QueueType::graphics);
    CHECK(graphics_queue_ret);
    graphics_queue = graphics_queue_ret.value();

    auto present_queue_ret =
        bootstrap.device.get_queue(vkb::QueueType::present);
    CHECK(present_queue_ret);
    present_queue = present_queue_ret.value();
  }

  void init_render_pass(BootstrapInfo &bootstrap) {
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = bootstrap.swapchain.image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                               VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    CHECK_VK(bootstrap.dispatch.createRenderPass(&render_pass_info, nullptr,
                                                 &render_pass));
  }

  void init_graphics_pipeline(BootstrapInfo &bootstrap) {
    auto vert = create_shader_module(bootstrap, "./build/shader.vert.spv");
    auto frag = create_shader_module(bootstrap, "./build/shader.frag.spv");

    VkPipelineShaderStageCreateInfo vert_stage_info = {};
    vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = vert;
    vert_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info = {};
    frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = frag;
    frag_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_stage_info,
                                                       frag_stage_info};

    auto binding_description = Vertex::binding_description();
    auto attribute_descriptions = Vertex::attribute_descriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.vertexAttributeDescriptionCount =
        attribute_descriptions.size();
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.pVertexAttributeDescriptions =
        attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)bootstrap.swapchain.extent.width;
    viewport.height = (float)bootstrap.swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = bootstrap.swapchain.extent;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &colorBlendAttachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pushConstantRangeCount = 0;

    CHECK_VK(bootstrap.dispatch.createPipelineLayout(&pipeline_layout_info,
                                                     NULL, &pipeline_layout));
    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_info = {};
    dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_info.dynamicStateCount =
        static_cast<uint32_t>(dynamic_states.size());
    dynamic_info.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_info;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    // TODO: add pipeline caching
    CHECK_VK(bootstrap.dispatch.createGraphicsPipelines(
        VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline));

    bootstrap.dispatch.destroyShaderModule(vert, NULL);
    bootstrap.dispatch.destroyShaderModule(frag, NULL);
  }

  void init_frame_data(BootstrapInfo &bootstrap) {
    frames.clear();
    frames.resize(bootstrap.swapchain.image_count);

    auto images = bootstrap.swapchain.get_images().value();
    auto image_views = bootstrap.swapchain.get_image_views().value();

    for (size_t i = 0; i < bootstrap.swapchain.image_count; i++) {
      FrameData frame_data;

      frame_data.swapchain_image = images[i];
      frame_data.swapchain_image_view = image_views[i];

      VkFramebufferCreateInfo framebuffer_create_info = {};
      framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_create_info.renderPass = render_pass;
      framebuffer_create_info.attachmentCount = 1;
      framebuffer_create_info.pAttachments = &frame_data.swapchain_image_view;
      framebuffer_create_info.width = bootstrap.swapchain.extent.width;
      framebuffer_create_info.height = bootstrap.swapchain.extent.height;
      framebuffer_create_info.layers = 1;

      CHECK_VK(bootstrap.dispatch.createFramebuffer(
          &framebuffer_create_info, NULL, &frame_data.framebuffer));

      frame_data.image_in_flight = VK_NULL_HANDLE;

      frames[i] = frame_data;
    }
  }

  void init_frames_in_flight(BootstrapInfo &bootstrap) {
    // TODO: cleanup
    frames_in_flight.clear();
    frames_in_flight.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_create = {};
    semaphore_create.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_create = {};
    fence_create.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < frames_in_flight.size(); i++) {
      FrameInFlight finf;

      CHECK_VK(bootstrap.dispatch.createSemaphore(&semaphore_create, NULL,
                                                  &finf.finished_semaphore));
      CHECK_VK(bootstrap.dispatch.createSemaphore(&semaphore_create, NULL,
                                                  &finf.available_semaphore));
      CHECK_VK(bootstrap.dispatch.createFence(&fence_create, NULL,
                                              &finf.in_flight_fence));

      frames_in_flight[i] = finf;
    }
  }

  void init_command_pool(BootstrapInfo &bootstrap) {
    VkCommandPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.queueFamilyIndex =
        bootstrap.device.get_queue_index(vkb::QueueType::graphics).value();

    CHECK_VK(bootstrap.dispatch.createCommandPool(&create_info, NULL,
                                                  &command_pool));
  }

  void write_command_buffer(BootstrapInfo &bootstrap,
                            ImDrawData *imgui_draw_data = nullptr) {
    // XXX: don't write ImGUI's data to each buffer every time

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = frames.size();

    std::vector<VkCommandBuffer> command_buffers(alloc_info.commandBufferCount);
    CHECK_VK(bootstrap.dispatch.allocateCommandBuffers(&alloc_info,
                                                       command_buffers.data()));

    for (uint32_t i = 0; i < command_buffers.size(); i++) {
      VkCommandBufferBeginInfo begin_info = {};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      // XXX: fixes validation layers screaming when resizing, needs further
      // investigation
      begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

      CHECK_VK(bootstrap.dispatch.beginCommandBuffer(command_buffers[i],
                                                     &begin_info));

      VkRenderPassBeginInfo render_pass_info = {};
      render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      render_pass_info.renderPass = render_pass;
      render_pass_info.framebuffer = frames[i].framebuffer;
      render_pass_info.renderArea.offset = {0, 0};
      render_pass_info.renderArea.extent = bootstrap.swapchain.extent;
      VkClearValue clearColor{{{0.0f, 0.0f, 0.0f, 1.0f}}};
      render_pass_info.clearValueCount = 1;
      render_pass_info.pClearValues = &clearColor;

      VkViewport viewport = {};
      viewport.x = 0.0f;
      viewport.y = 0.0f;
      viewport.width = (float)bootstrap.swapchain.extent.width;
      viewport.height = (float)bootstrap.swapchain.extent.height;
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;

      VkRect2D scissor = {};
      scissor.offset = {0, 0};
      scissor.extent = bootstrap.swapchain.extent;

      bootstrap.dispatch.cmdSetViewport(command_buffers[i], 0, 1, &viewport);
      bootstrap.dispatch.cmdSetScissor(command_buffers[i], 0, 1, &scissor);

      bootstrap.dispatch.cmdBeginRenderPass(
          command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

      bootstrap.dispatch.cmdBindPipeline(
          command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

      VkBuffer vertex_buffers[] = {mesh->vert_buffer};
      VkDeviceSize offsets[] = {0};
      bootstrap.dispatch.cmdBindVertexBuffers(command_buffers[i], 0, 1,
                                              vertex_buffers, offsets);

      bootstrap.dispatch.cmdDraw(command_buffers[i], 3, 1, 0, 0);

      // TODO: fix imgui
      if (false)
        ImGui_ImplVulkan_RenderDrawData(imgui_draw_data, command_buffers[i]);

      bootstrap.dispatch.cmdEndRenderPass(command_buffers[i]);

      CHECK_VK(bootstrap.dispatch.endCommandBuffer(command_buffers[i]));

      frames[i].command_buffer = command_buffers[i];
    }
  }

  void recreate_swapchain(BootstrapInfo &bootstrap) {
    bootstrap.dispatch.deviceWaitIdle();
    bootstrap.dispatch.destroyCommandPool(command_pool, nullptr);

    std::vector<VkImageView> image_views;
    for (auto &frame : frames) {
      bootstrap.dispatch.destroyFramebuffer(frame.framebuffer, nullptr);
      image_views.push_back(frame.swapchain_image_view);
    }

    bootstrap.swapchain.destroy_image_views(image_views);

    bootstrap.init_swapchain();
    init_frame_data(bootstrap);
    init_command_pool(bootstrap);
    write_command_buffer(bootstrap);
  }

  void draw_frame(BootstrapInfo &bootstrap) {
    CHECK_VK(bootstrap.dispatch.waitForFences(
        1, &frames_in_flight[current_frame].in_flight_fence, VK_TRUE,
        UINT64_MAX));

    uint32_t image_index = 0;
    VkResult result = bootstrap.dispatch.acquireNextImageKHR(
        bootstrap.swapchain, UINT64_MAX,
        frames_in_flight[current_frame].available_semaphore, VK_NULL_HANDLE,
        &image_index);

    if (result != VK_SUBOPTIMAL_KHR) {
      if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain(bootstrap);
        return;
      } else {
        CHECK_VK(result);
      }
    }

    if (frames[image_index].image_in_flight != VK_NULL_HANDLE) {
      CHECK_VK(bootstrap.dispatch.waitForFences(
          1, &frames[image_index].image_in_flight, VK_TRUE, UINT64_MAX));
    }

    frames[image_index].image_in_flight =
        frames_in_flight[current_frame].in_flight_fence;

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores =
        &frames_in_flight[current_frame].available_semaphore;
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &frames[image_index].command_buffer;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores =
        &frames_in_flight[current_frame].finished_semaphore;

    CHECK_VK(bootstrap.dispatch.resetFences(
        1, &frames_in_flight[current_frame].in_flight_fence));

    CHECK_VK(bootstrap.dispatch.queueSubmit(
        graphics_queue, 1, &submit_info,
        frames_in_flight[current_frame].in_flight_fence));

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores =
        &frames_in_flight[current_frame].finished_semaphore;

    VkSwapchainKHR swapChains[] = {bootstrap.swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapChains;

    present_info.pImageIndices = &image_index;

    result = bootstrap.dispatch.queuePresentKHR(present_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
      recreate_swapchain(bootstrap);
      return;
    } else {
      CHECK_VK(result);
    }

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void init_imgui(BootstrapInfo &bootstrap) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    CHECK_VK(vkCreateDescriptorPool(bootstrap.device, &pool_info, NULL,
                                    &imgui_descriptor_pool));

    ImGui_ImplGlfw_InitForVulkan(bootstrap.window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = bootstrap.instance;
    init_info.PhysicalDevice = bootstrap.physical_device;
    init_info.Device = bootstrap.device;
    init_info.Queue = graphics_queue;
    init_info.QueueFamily =
        bootstrap.device.get_queue_index(vkb::QueueType::graphics).value();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = imgui_descriptor_pool;
    init_info.RenderPass = render_pass;
    init_info.Subpass = 0;
    init_info.MinImageCount = bootstrap.swapchain.requested_min_image_count;
    init_info.ImageCount = bootstrap.swapchain.image_count;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&init_info);
  }

  void init(BootstrapInfo &bootstrap) {
    init_queues(bootstrap);
    init_render_pass(bootstrap);
    init_graphics_pipeline(bootstrap);
    init_frame_data(bootstrap);
    init_frames_in_flight(bootstrap);
    init_command_pool(bootstrap);
    init_imgui(bootstrap);
  }
};

} // namespace b::engine
