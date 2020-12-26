#pragma once

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <cstdint>

namespace qz::gfx {
    struct Window {
    private:
        GLFWwindow* _handle;
        std::uint32_t _width;
        std::uint32_t _height;

        qz_nodiscard Window(GLFWwindow*, std::uint32_t, std::uint32_t) noexcept;
    public:
        qz_nodiscard static Window create(std::uint32_t, std::uint32_t, const char*) noexcept;
        static void destroy(Window&) noexcept;
        static void terminate() noexcept;

        Window() noexcept = default;

        qz_nodiscard bool should_close() const noexcept;
        qz_nodiscard GLFWwindow* handle() const noexcept;
        qz_nodiscard std::uint32_t width() const noexcept;
        qz_nodiscard std::uint32_t height() const noexcept;
    };

    qz_nodiscard double get_time() noexcept;
    void poll_events() noexcept;
} // namespace qz::gfx