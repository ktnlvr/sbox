#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "bootstrap.hpp"
#include "vertex.hpp"

namespace b::engine {

struct Mesh {
  VkBuffer vert_buffer;
  VmaAllocation allocation;
};

struct Library {
  Mesh add_mesh(BootstrapInfo &bootstrap, std::vector<Vertex> vertices) {
    Mesh mesh = {};

    VkBufferCreateInfo vert_buffer_info = {};
    vert_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vert_buffer_info.size = vertices.size() * sizeof(Vertex);
    vert_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vert_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vmalloc_info = {};
    vmalloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    vmalloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    CHECK_VK(vmaCreateBuffer(bootstrap.allocator, &vert_buffer_info,
                             &vmalloc_info, &mesh.vert_buffer, &mesh.allocation,
                             nullptr));
    CHECK_VK(vmaCopyMemoryToAllocation(bootstrap.allocator, vertices.data(),
                              mesh.allocation, 0,
                              sizeof(Vertex) * vertices.size()));

    return mesh;
  }
};

} // namespace b::engine
