#pragma once

#include <spdlog/spdlog.h>
#include <vulkan/vk_enum_string_helper.h>

#define CHECK(expr)                                                            \
  if (!(expr)) {                                                               \
    spdlog::error("{}:{} {}", __FILE__, __LINE__, #expr);                      \
    exit(-2);                                                                  \
  }
#define CHECK_VK(expr)                                                         \
  [&]() {                                                                      \
    VkResult result = expr;                                                    \
    if (result < 0) {                                                          \
      spdlog::error("{}:{} {} {}", __FILE__, __LINE__,                         \
                    string_VkResult(result), #expr);                           \
    } else if (result > 0) {                                                   \
      spdlog::warn("{}:{} {} = {} ({})", __FILE__, __LINE__, #expr,            \
                   string_VkResult(result), result);                           \
    }                                                                          \
  }();
#define ASSERT(expr, ...)
