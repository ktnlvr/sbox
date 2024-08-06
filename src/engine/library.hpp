#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "bootstrap.hpp"
#include "vertex.hpp"

namespace b::engine {

struct Mesh {
  VkBuffer vert_buffer;
  VmaAllocation vert_allocation;
  VkBuffer index_buffer;
  VmaAllocation index_allocation;
};

struct Library {
  Mesh add_mesh(BootstrapInfo &bootstrap, std::vector<Vertex> vertices,
                std::vector<uint32_t> indices) {
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
                             &vmalloc_info, &mesh.vert_buffer,
                             &mesh.vert_allocation, nullptr));
    CHECK_VK(vmaCopyMemoryToAllocation(bootstrap.allocator, vertices.data(),
                                       mesh.vert_allocation, 0,
                                       sizeof(Vertex) * vertices.size()));

    VkBufferCreateInfo index_buffer_info = {};
    index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    index_buffer_info.size = sizeof(uint32_t) * indices.size();
    index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    index_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CHECK_VK(vmaCreateBuffer(bootstrap.allocator, &index_buffer_info,
                             &vmalloc_info, &mesh.index_buffer,
                             &mesh.index_allocation, nullptr));
    CHECK_VK(vmaCopyMemoryToAllocation(bootstrap.allocator, indices.data(),
                                       mesh.index_allocation, 0,
                                       sizeof(uint32_t) * indices.size()));

    return mesh;
  }
};

} // namespace b::engine
