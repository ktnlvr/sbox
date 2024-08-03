#include <iostream>

#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <spdlog/spdlog.h>

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

int main(void) {
  vkb::InstanceBuilder instance_builder;
  instance_builder.request_validation_layers()
      .set_app_name("Sandbox")
      .set_engine_name("Sandbox Vulkan Engine")
      .require_api_version(1, 0, 0);
  instance_builder.set_debug_callback(
      [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
         VkDebugUtilsMessageTypeFlagsEXT _messageType,
         const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
         void *_pUserData) -> VkBool32 {
        // TODO: propert severity handling
        if (messageSeverity >=
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
          spdlog::info(pCallbackData->pMessage);
        }
        return VK_FALSE;
      });

  auto instance_result = instance_builder.build();

  if (!instance_result) {
    spdlog::error(instance_result.error().message());
  }

  vkb::Instance instance = instance_result.value();
  GLFWwindow *window = GLFWwindow_create("Sandbox");
  GLFWwindow_show(window);

  return 0;
}
