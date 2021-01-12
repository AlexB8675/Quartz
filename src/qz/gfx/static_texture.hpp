#pragma once

#include <qz/gfx/image.hpp>

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <string_view>

namespace qz::gfx {
    class StaticTexture {
        Image _handle;
    public:
        qz_nodiscard static StaticTexture from_raw(const Image&) noexcept;
        qz_nodiscard static meta::Handle<StaticTexture> allocate(const Context&, std::string_view, VkFormat = VK_FORMAT_R8G8B8A8_SRGB) noexcept;
        qz_nodiscard static meta::Handle<StaticTexture> request(const Context&, std::string_view, VkFormat = VK_FORMAT_R8G8B8A8_SRGB) noexcept;
        static void destroy(const Context&, StaticTexture&) noexcept;

        qz_nodiscard VkImageView view() const noexcept;
    };
} // namespace qz::gfx