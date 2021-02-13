#include <qz/gfx/window.hpp>

#include <GLFW/glfw3.h>

#include <iostream>

namespace qz::gfx {
    qz_nodiscard Window::Window(GLFWwindow* handle, std::uint32_t width, std::uint32_t height) noexcept
        : _handle(handle),
          _width(width),
          _height(height),
          _mouse_off(),
          _last_pos(),
          _captured(false) {
        glfwSetWindowUserPointer(handle, this);
    }

    qz_nodiscard Window Window::create(std::uint32_t width, std::uint32_t height, const char* title) noexcept {
        qz_assert(glfwInit(), "GLFW failed to initialize, or was not initialized correctly");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, false);

        auto window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        qz_assert(window, "failed to create window.");

        glfwSetCursorPosCallback(window, [](GLFWwindow* handle, double x, double y) {
            auto& window = *static_cast<Window* const>(glfwGetWindowUserPointer(handle));
            qz_likely_if(window._captured) {
                constexpr auto sensitivity = 0.15f;
                window._moved = true;
                window._mouse_off = {
                    (x - window._last_pos.x) * sensitivity,
                    (window._last_pos.y - y) * sensitivity
                };
                window._last_pos = { x, y };
            }
        });

        return {
            window,
            width,
            height
        };
    }

    void Window::destroy(Window& window) noexcept {
        glfwDestroyWindow(window._handle);
        window = {};
    }

    void Window::terminate() noexcept {
        glfwTerminate();
    }

    qz_nodiscard bool Window::should_close() const noexcept {
        return glfwWindowShouldClose(_handle);
    }

    qz_nodiscard KeyState Window::get_key_state(Keys key) const noexcept {
        switch (glfwGetKey(_handle, static_cast<int>(key))) {
            case GLFW_PRESS:   return KeyState::pressed;
            case GLFW_RELEASE: return KeyState::released;
        }
        qz_unreachable();
    }

    qz_nodiscard KeyState Window::get_mouse_state(Mouse key) const noexcept {
        switch (glfwGetMouseButton(_handle, static_cast<int>(key))) {
            case GLFW_PRESS:   return KeyState::pressed;
            case GLFW_RELEASE: return KeyState::released;
        }
        qz_unreachable();
    }

    qz_nodiscard GLFWwindow* Window::handle() const noexcept {
        return _handle;
    }

    qz_nodiscard std::uint32_t Window::width() const noexcept {
        return _width;
    }

    qz_nodiscard std::uint32_t Window::height() const noexcept {
        return _height;
    }

    void Window::poll_events() noexcept {
        _moved = false;
        glfwPollEvents();
        if (get_mouse_state(Mouse::right_button) == KeyState::pressed) {
            if (!_captured) {
                _captured = true;
                _last_pos = { _width / 2.0, _height / 2.0 };
                glfwSetCursorPos(_handle, _last_pos.x, _last_pos.y);
                glfwSetInputMode(_handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        } else if (_captured) {
            glfwSetInputMode(_handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            _captured = false;
        }
    }

    Point Window::get_mouse_offset() const noexcept {
        return _moved ? _mouse_off : Point{};
    }

    qz_nodiscard double get_time() noexcept {
        return glfwGetTime();
    }
} // namespace qz::gfx