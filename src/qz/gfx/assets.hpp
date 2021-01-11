#pragma once

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <qz/meta/types.hpp>

namespace qz::assets {
    template <typename T>
    qz_nodiscard meta::Handle<T> emplace_empty() noexcept;

    template <typename T>
    qz_nodiscard T& from_handle(meta::Handle<T>) noexcept; // Externally synchronized

    template <typename T>
    qz_nodiscard bool is_ready(meta::Handle<T>) noexcept; // Externally synchronized

    template <typename T>
    void finalize(meta::Handle<T>, T&&) noexcept;

    void free_all_resources(const gfx::Context&) noexcept;

    gfx::StaticTexture& default_texture() noexcept; // Externally synchronized

    template <typename T>
    void lock() noexcept;

    template <typename T>
    void unlock() noexcept;
} // namespace qz::assets