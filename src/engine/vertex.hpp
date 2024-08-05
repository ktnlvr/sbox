#pragma once

#include <array>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace b::engine
{

struct Vertex {
    glm::vec2 position;
    glm::vec3 color;

    static VkVertexInputBindingDescription binding_description() {
        VkVertexInputBindingDescription description;
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        description.stride = sizeof(Vertex);

        return description;
    }

    static std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions() {
        std::array<VkVertexInputAttributeDescription, 2> descriptions;
        descriptions[0].binding = 0;
        descriptions[0].location = 0;
        descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        descriptions[0].offset = offsetof(Vertex, position);

        descriptions[1].binding = 0;
        descriptions[1].location = 1;
        descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        descriptions[1].offset = offsetof(Vertex, color);

        return descriptions;
    }
};

const std::array<Vertex, 3> TRIANGLE_VERTS = {
    Vertex { .position = {0, -0.5}, .color = {1., 0., 0.}},
    Vertex { .position = {0.5, 0.5}, .color = {0., 1., 0.}},
    Vertex { .position = {-0.5, 0.5}, .color = {0., 0., 1.}},
};

} // namespace b::engine
