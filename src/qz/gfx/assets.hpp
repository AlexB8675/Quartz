#pragma once

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <qz/meta/types.hpp>

#include <vector>
#include <mutex>

namespace qz::assets {
    template <typename T>
    qz_nodiscard meta::Handle<T> emplace_empty() noexcept;

    template <typename T>
    qz_nodiscard std::unique_lock<std::mutex> acquire() noexcept;

    template <typename T>
    qz_nodiscard T& from_handle(meta::Handle<T>) noexcept;

    template <typename T>
    qz_nodiscard bool is_ready(meta::Handle<T>) noexcept;

    template <typename T>
    void finalize(meta::Handle<T>, T&&) noexcept;

    void free_all_resources(const gfx::Context&) noexcept;

    qz_nodiscard gfx::StaticTexture& default_texture() noexcept;

    qz_nodiscard std::vector<VkDescriptorImageInfo> all_textures(const gfx::Context&) noexcept;
} // namespace qz::assets