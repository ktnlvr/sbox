#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <spdlog/spdlog.h>

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
  if (!ret) {
    spdlog::error("GLFW Error {}: ", ret, err_message);
    // TODO: possibly free the err_message?
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

  VkSurfaceKHR create_surface() {
    CHECK_FMT(instance != VK_NULL_HANDLE,
           "The instance has not been initialized yet. Try initializing the "
           "instance before surface creation");
    CHECK_FMT(window != nullptr, "The window has not been created yet. Did GLFW "
                              "return an error upon window creation?");

    // TODO: check that instance and swapchain are correct
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult result = glfwCreateWindowSurface(instance, window, NULL, &surface);
    if (result) {
      CHECK(GLFW_check_error());
    }

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
    auto physical_device_ret =
        physical_device_selector.set_surface(surface).select(
            vkb::DeviceSelectionMode::only_fully_suitable);
    CHECK(physical_device_ret);
    physical_device = physical_device_ret.value();

    vkb::DeviceBuilder device_builder(physical_device);
    auto device_ret = device_builder.build();
    CHECK(device_ret);
    device = device_ret.value();

    dispatch = device.make_table();
  }

  void init_swapchain() {
    vkb::SwapchainBuilder swapchain_builder(device);
    auto swapchain_ret = swapchain_builder.set_old_swapchain(swapchain).build();
    CHECK(swapchain_ret);
    vkb::destroy_swapchain(swapchain);
    swapchain = swapchain_ret.value();
  }

  uint32_t find_memory(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    auto mem = physical_device.memory_properties;
    for (size_t i = 0; i < mem.memoryTypeCount; i++) {
      // MAGIC: check the docs
      if ((type_filter & (1 << i)) &&
          ((mem.memoryTypes[i].propertyFlags & properties) == properties)) {
        return i;
      }
    }

    PANIC("Requested memory type not found");
    return -1;
  }

  void init() {
    init_device();
    init_swapchain();
  }
};

} // namespace engine

} // namespace b
