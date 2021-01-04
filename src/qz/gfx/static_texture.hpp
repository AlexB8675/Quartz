#pragma once

#include <qz/gfx/image.hpp>

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <string_view>

namespace qz::gfx {
    struct DescriptorImageInfo {
        VkImageView handle;
        std::size_t index;
    };

    class StaticTexture {
        Image _handle;
        std::size_t _index;
    public:
        qz_nodiscard static StaticTexture from_raw(const Image&, std::size_t) noexcept;
        qz_nodiscard static meta::Handle<StaticTexture> allocate(const Context&, std::string_view) noexcept;
        qz_nodiscard static meta::Handle<StaticTexture> request(const Context&, std::string_view) noexcept;
        static void destroy(const Context&, StaticTexture&) noexcept;

        qz_nodiscard DescriptorImageInfo info() const noexcept;
    };
} // namespace qz::gfx