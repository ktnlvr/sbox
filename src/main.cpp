#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <spdlog/spdlog.h>

#include "engine/rendering.hpp"

using namespace b;

int main(void) {
  engine::BootstrapInfo bootstrap;
  bootstrap.init();
  engine::GLFWwindow_show(bootstrap.window);

  engine::RenderData render_data;
  render_data.init(bootstrap);

  while (!glfwWindowShouldClose(bootstrap.window)) {
    glfwPollEvents();
    render_data.draw_frame(bootstrap);
  }

  return 0;
}
