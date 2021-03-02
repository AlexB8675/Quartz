#pragma once

#include <qz/meta/constants.hpp>

#include <qz/util/macros.hpp>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <cstdlib>
#include <array>

namespace qz::meta {
    template <typename T>
    using in_flight_array_t = std::array<T, in_flight>;

    template <typename T>
    struct Handle {
        std::size_t index;
    };

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normals;
        glm::vec2 uvs;
        glm::vec3 tangents;
        glm::vec3 bitangents;

        qz_make_equal_to(Vertex, position, normals, uvs, tangents, bitangents);
    };
} // namespace qz::meta