#include <qz/gfx/window.hpp>

#include <GLFW/glfw3.h>

namespace qz::gfx {
    qz_nodiscard Window::Window(GLFWwindow* handle, std::uint32_t width, std::uint32_t height) noexcept
        : _handle(handle),
          _width(width),
          _height(height) {
        glfwSetWindowUserPointer(handle, this);
    }

    qz_nodiscard Window Window::create(std::uint32_t width, std::uint32_t height, const char* title) noexcept {
        qz_assert(glfwInit(), "GLFW failed to initialize, or was not initialized correctly");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, false);

        auto window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        qz_assert(window, "Failed to create window.");

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

    qz_nodiscard GLFWwindow* Window::handle() const noexcept {
        return _handle;
    }

    qz_nodiscard std::uint32_t Window::width() const noexcept {
        return _width;
    }

    qz_nodiscard std::uint32_t Window::height() const noexcept {
        return _height;
    }

    qz_nodiscard double get_time() noexcept {
        return glfwGetTime();
    }

    void poll_events() noexcept {
        glfwPollEvents();
    }
} // namespace qz::gfx