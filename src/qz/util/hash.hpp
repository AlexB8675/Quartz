#pragma once

#include <qz/gfx/pipeline.hpp>
#include <qz/util/macros.hpp>

#include <type_traits>
#include <utility>
#include <cstdint>
#include <cstring>
#include <vector>
#include <map>

namespace qz::util {
    template <typename... Args>
    qz_nodiscard std::size_t hash(std::size_t seed, Args&&... args) noexcept {
        return ((seed ^= std::hash<std::remove_cvref_t<Args>>()(args) + 0x9e3779b9 + (seed << 6) + (seed >> 2)), ...);
    }
} // namespace qz::util

namespace std {
    template <typename Ty>
    struct hash<std::vector<Ty>> {
        qz_nodiscard size_t operator ()(const std::vector<Ty>& vec) const noexcept {
            size_t result = 0;
            for (const auto& each : vec) {
                result = qz::util::hash(result, each);
            }
            return result;
        }
    };

    qz_make_hashable(qz::gfx::DescriptorBinding, name, index, count, type, stage);
} // namespace std
