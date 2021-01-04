#pragma once

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <qz/gfx/static_buffer.hpp>

#include <cstdint>
#include <vector>

namespace qz::gfx {
    struct StaticMesh {
        struct CreateInfo {
            std::vector<float> geometry;
            std::vector<std::uint32_t> indices;
        };
        StaticBuffer geometry;
        StaticBuffer indices;

        qz_nodiscard static meta::Handle<StaticMesh> request(const Context&, StaticMesh::CreateInfo&&) noexcept;
        static void destroy(const Context&, StaticMesh&) noexcept;
    };
} // namespace qz::gfx