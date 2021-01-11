#pragma once

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <cstdint>

namespace qz::gfx {
    enum class Keys : int {
        space      = 32,
        A          = 65,
        B          = 66,
        C          = 67,
        D          = 68,
        E          = 69,
        F          = 70,
        G          = 71,
        H          = 72,
        I          = 73,
        J          = 74,
        K          = 75,
        L          = 76,
        M          = 77,
        N          = 78,
        O          = 79,
        P          = 80,
        Q          = 81,
        R          = 82,
        S          = 83,
        T          = 84,
        U          = 85,
        V          = 86,
        W          = 87,
        X          = 88,
        Y          = 89,
        Z          = 90,
        right      = 262,
        left       = 263,
        down       = 264,
        up         = 265,
        left_shift = 340
    };

    enum class Mouse : int {
        left_button   = 0,
        right_button  = 1,
        middle_button = 2
    };

    enum class KeyState {
        pressed,
        released
    };

    struct Point {
        double x;
        double y;
    };

    struct Window {
    private:
        GLFWwindow* _handle;
        std::uint32_t _width;
        std::uint32_t _height;
        Point _mouse_off;
        Point _last_pos;
        bool _captured;
        bool _moved;

        qz_nodiscard Window(GLFWwindow*, std::uint32_t, std::uint32_t) noexcept;
    public:
        qz_nodiscard static Window create(std::uint32_t, std::uint32_t, const char*) noexcept;
        static void destroy(Window&) noexcept;
        static void terminate() noexcept;

        qz_nodiscard Window() noexcept = default;

        qz_nodiscard bool should_close() const noexcept;
        qz_nodiscard KeyState get_key_state(Keys) const noexcept;
        qz_nodiscard KeyState get_mouse_state(Mouse) const noexcept;
        qz_nodiscard GLFWwindow* handle() const noexcept;
        qz_nodiscard std::uint32_t width() const noexcept;
        qz_nodiscard std::uint32_t height() const noexcept;
        qz_nodiscard Point get_mouse_offset() const noexcept;
        void poll_events() noexcept;
    };

    qz_nodiscard double get_time() noexcept;
} // namespace qz::gfx