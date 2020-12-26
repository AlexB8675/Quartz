#pragma once

#include <vulkan/vulkan.h>

#include <qz/gfx/context.hpp>
#include <qz/gfx/window.hpp>

#include <qz/util/macros.hpp>

#include <vector>

namespace qz::gfx {
    struct Swapchain {
        VkSurfaceKHR surface;
        VkSwapchainKHR handle;
        VkExtent2D extent;
        VkFormat format;
        std::vector<Image> images;

        qz_nodiscard static Swapchain create(const Context&, const Window&) noexcept;
        static void destroy(const Context&, Swapchain&) noexcept;

        qz_nodiscard const Image& operator [](std::size_t) const noexcept;
    };
} // namespace qz::gfx