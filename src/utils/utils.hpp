#pragma once

#include <spdlog/spdlog.h>
#include <vulkan/vk_enum_string_helper.h>

#define CHECK(expr)                                                            \
  if (!(expr)) {                                                               \
    spdlog::error("{}:{} ({})", __FILE__, __LINE__, #expr);                    \
    exit(-2);                                                                  \
  }

#define CHECK_REPORT_STR(expr, msg)                                            \
  if (!(expr)) {                                                               \
    spdlog::error("{}:{} ({})" msg, __FILE__, __LINE__, #expr);                \
    exit(-2);                                                                  \
  }

#define CHECK_REPORT_FMT(expr, msg, ...)                                       \
  if (!(expr)) {                                                               \
    spdlog::error("{}:{} ({}) " msg, __FILE__, __LINE__, #expr, __VA_ARGS__);  \
    exit(-2);                                                                  \
  }

#define CHECK_VK(expr)                                                         \
  [&]() {                                                                      \
    VkResult result##__LINE__ = expr;                                          \
    if (result##__LINE__ == VK_SUCCESS)                                        \
      return;                                                                  \
    if (result##__LINE__ < 0) {                                                \
      spdlog::error("{}:{} ({}) = {}", __FILE__, __LINE__,                     \
                    string_VkResult(result##__LINE__), #expr);                 \
    } else if (result##__LINE__ > 0) {                                         \
      spdlog::warn("{}:{} ({}) = {}", __FILE__, __LINE__, #expr,               \
                   string_VkResult(result##__LINE__));                         \
    }                                                                          \
  }()
#define PANIC(...) spdlog::error("{}:{} ", __FILE__, __LINE__, __VA_ARGS__)
