#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <spdlog/spdlog.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "utils/utils.hpp"

namespace b {

namespace engine {

GLFWwindow *GLFWwindow_create(const char *window_name = "",
                              bool resize = false) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  if (!resize)
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  return glfwCreateWindow(1024, 1024, window_name, NULL, NULL);
}

void GLFWwindow_show(GLFWwindow *window) { glfwShowWindow(window); }

int GLFW_check_error() {
  const char *err_message = nullptr;
  int ret = glfwGetError(&err_message);
  if (err_message) {
    CHECK_REPORT_FMT(ret, "{}", err_message);
  }

  return ret;
}

struct BootstrapInfo {
  GLFWwindow *window;
  vkb::Instance instance;
  vkb::InstanceDispatchTable instance_dispatch;
  VkSurfaceKHR surface;
  vkb::PhysicalDevice physical_device;
  vkb::Device device;
  vkb::DispatchTable dispatch;
  vkb::Swapchain swapchain;
  VmaAllocator allocator;
  VkCommandPool immediate_command_pool;

  VkSurfaceKHR create_surface() {
    CHECK_REPORT_STR(
        instance != VK_NULL_HANDLE,
        "The instance has not been initialized yet. Try initializing the "
        "instance before surface creation");
    CHECK_REPORT_STR(window != nullptr,
                     "The window has not been created yet. Did GLFW "
                     "return an error upon window creation?");
    CHECK(instance);

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    int result = glfwCreateWindowSurface(instance, window, NULL, &surface);
    GLFW_check_error();

    return surface;
  }

  void init_device() {
    window = GLFWwindow_create("VkSandbox");

    vkb::InstanceBuilder instance_builder;
    instance_builder.request_validation_layers()
        .set_app_name("Sandbox")
        .set_engine_name("Sandbox Vulkan Engine")
        .require_api_version(1, 2, 0)
        .set_debug_callback(
            [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT _messageType,
               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
               void *_pUserData) -> VkBool32 {
              // TODO: propert severity handling
              if (messageSeverity ==
                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                spdlog::warn(pCallbackData->pMessage);
              } else if (messageSeverity >=
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
                spdlog::error(pCallbackData->pMessage);
              } else {
                spdlog::info(pCallbackData->pMessage);
              }
              return VK_FALSE;
            });

    auto system_info = vkb::SystemInfo::get_system_info();
    CHECK(system_info);

    auto instance_ret = instance_builder.build();
    CHECK(instance_ret);

    instance = instance_ret.value();
    instance_dispatch = instance.make_table();
    surface = create_surface();

    vkb::PhysicalDeviceSelector physical_device_selector(instance);
    auto physical_device_ret = physical_device_selector.set_surface(surface)
                                   .prefer_gpu_device_type()
                                   .allow_any_gpu_device_type(false)
                                   .select();
    CHECK(physical_device_ret);
    physical_device = physical_device_ret.value();
    spdlog::info("Using {}", physical_device.name);

    vkb::DeviceBuilder device_builder(physical_device);
    auto device_ret = device_builder.build();
    CHECK(device_ret);
    device = device_ret.value();

    dispatch = device.make_table();
  }

  void init_swapchain() {
    vkb::SwapchainBuilder swapchain_builder(device);
    // TODO: select at runtime/compile time
    auto swapchain_ret =
        swapchain_builder.set_old_swapchain(swapchain)
            .set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
            .build();
    CHECK(swapchain_ret);
    vkb::destroy_swapchain(swapchain);
    swapchain = swapchain_ret.value();
  }

  void init_memory() {
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_2;
    allocator_info.instance = instance;
    allocator_info.device = device;
    allocator_info.physicalDevice = physical_device;

    CHECK_VK(vmaCreateAllocator(&allocator_info, &allocator));
  }

  void init_immediate_command_pool() {
    CHECK(device);

    auto present_queue_ret = device.get_queue_index(vkb::QueueType::graphics);
    CHECK(present_queue_ret);

    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    pool_info.queueFamilyIndex = present_queue_ret.value();

    CHECK_VK(
        dispatch.createCommandPool(&pool_info, NULL, &immediate_command_pool));
  }

  // TODO: accept any callable
  void submit_immediate_command_buffer(
      vkb::QueueType queue_type,
      std::function<VkResult(VkCommandBuffer)> record) {
    VkCommandBufferAllocateInfo command_buffer_alloc_info = {};
    command_buffer_alloc_info.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandPool = immediate_command_pool;
    command_buffer_alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    CHECK_VK(dispatch.allocateCommandBuffers(&command_buffer_alloc_info,
                                             &command_buffer));

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    CHECK_VK(dispatch.beginCommandBuffer(command_buffer, &begin_info));

    CHECK_VK(record(command_buffer));

    CHECK_VK(dispatch.endCommandBuffer(command_buffer));

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    auto queue_ret = device.get_queue(queue_type);
    // TODO: error check this
    auto queue = queue_ret.value();

    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
  }

  void init() {
    init_device();
    init_swapchain();
    init_memory();
    init_immediate_command_pool();
  }
};

} // namespace engine

} // namespace b
