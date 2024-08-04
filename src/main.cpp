#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <spdlog/spdlog.h>

#include "engine/bootstrap.hpp"

using namespace b;

int main(void) {
  engine::BootstrapInfo bootstrap;
  bootstrap.init();
  engine::GLFWwindow_show(bootstrap.window);

  while (!glfwWindowShouldClose(bootstrap.window)) {
    glfwPollEvents();
  }

  return 0;
}
