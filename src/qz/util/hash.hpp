#pragma once

#include <qz/gfx/pipeline.hpp>

#include <qz/meta/types.hpp>

#include <qz/util/macros.hpp>

#include <glm/gtx/hash.hpp>

#include <type_traits>
#include <utility>
#include <cstdint>
#include <cstring>
#include <vector>

namespace qz::util {
    template <typename... Args>
    qz_nodiscard std::size_t hash(std::size_t seed, Args&&... args) noexcept {
        return ((seed ^= std::hash<std::remove_cvref_t<Args>>()(args) + 0x9e3779b9 + (seed << 6) + (seed >> 2)), ...);
    }
} // namespace qz::util

namespace std {
    qz_make_hashable(qz::gfx::DescriptorBinding, dynamic, name, index, count, type, stage);
    qz_make_hashable(VkDescriptorImageInfo, sampler, imageView, imageLayout);
    qz_make_hashable(qz::meta::Vertex, position, normals, uvs, tangents, bitangents);
    template <typename T>
    qz_make_hashable_pred(vector<T>, value, [&]() {
        size_t result = 0;
        for (const auto& each : value) {
            result = qz::util::hash(result, each);
        }
        return result;
    });
} // namespace std
