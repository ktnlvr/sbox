#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "bootstrap.hpp"
#include "vertex.hpp"

namespace b::engine {

struct Mesh {
  VkBuffer vert_buffer;
  VkDeviceMemory vert_buffer_memory;
};

struct Library {
  Mesh add_mesh(BootstrapInfo &bootstrap, std::vector<Vertex> vertices) {
    Mesh mesh = {};

    VkBufferCreateInfo vert_buffer_info = {};
    vert_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vert_buffer_info.size = vertices.size() * sizeof(Vertex);
    vert_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vert_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    bootstrap.dispatch.createBuffer(&vert_buffer_info, nullptr,
                                    &mesh.vert_buffer);

    VkMemoryRequirements requirements;
    bootstrap.dispatch.getBufferMemoryRequirements(mesh.vert_buffer,
                                                   &requirements);

    VkMemoryAllocateInfo malloc_info = {};
    malloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    malloc_info.allocationSize = requirements.size;
    malloc_info.memoryTypeIndex = bootstrap.find_memory(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    CHECK_VK(bootstrap.dispatch.allocateMemory(&malloc_info, nullptr,
                                               &mesh.vert_buffer_memory));

    bootstrap.dispatch.bindBufferMemory(mesh.vert_buffer,
                                        mesh.vert_buffer_memory, 0);
    void *data;
    bootstrap.dispatch.mapMemory(mesh.vert_buffer_memory, 0,
                                 vert_buffer_info.size, 0, &data);
    memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));
    bootstrap.dispatch.unmapMemory(mesh.vert_buffer_memory);

    return mesh;
  }
};

} // namespace b::engine
