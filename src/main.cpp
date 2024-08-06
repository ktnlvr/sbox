#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <spdlog/spdlog.h>

#include "engine/library.hpp"
#include "engine/rendering.hpp"

using namespace b;

int main(void) {
  engine::BootstrapInfo bootstrap;
  bootstrap.init();
  engine::GLFWwindow_show(bootstrap.window);

  engine::RenderData render_data;
  render_data.init(bootstrap);

  engine::Library library;
  auto mesh = library.add_mesh(
      bootstrap,
      std::vector(engine::VERTICES.cbegin(), engine::VERTICES.cend()),
      std::vector(engine::INDICES.cbegin(), engine::INDICES.cend()));
  engine::mesh = &mesh;

  render_data.write_command_buffer(bootstrap);

  while (!glfwWindowShouldClose(bootstrap.window)) {
    glfwPollEvents();

    ImGui_ImplVulkan_SetMinImageCount(
        bootstrap.swapchain.requested_min_image_count);
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    render_data.draw_frame(bootstrap);
  }

  return 0;
}
