#pragma once

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <qz/meta/types.hpp>

#include <string_view>
#include <vector>

namespace qz::gfx {
    struct TexturedMesh {
        meta::Handle<StaticMesh> mesh;
        meta::Handle<StaticTexture> diffuse;
        meta::Handle<StaticTexture> normal;
        meta::Handle<StaticTexture> spec;

        std::size_t vertex_count;
        std::size_t index_count;
    };

    struct StaticModel {
        std::vector<TexturedMesh> submeshes;

        qz_nodiscard static meta::Handle<StaticModel> request(const Context&, std::string_view) noexcept;
    };
} // namespace qz::gfx