#pragma once

#include <qz/util/macros.hpp>

#include <type_traits>
#include <algorithm>
#include <cstdint>

#include <vulkan/vulkan.h>

namespace qz::gfx {
    struct ClearColor {
        constexpr ClearColor() noexcept : value() {}
        template <typename... Args>
            requires (std::conjunction_v<std::is_convertible<Args, float>...>)
        constexpr explicit ClearColor(Args&&... args) noexcept : value{ .float32 = { std::forward<Args>(args)... } } {}

        VkClearColorValue value;
    };

    struct ClearDepth {
        constexpr ClearDepth() noexcept : value() {}
        constexpr ClearDepth(const float depth, const std::uint32_t stencil) noexcept : value{ depth, stencil } {}

        VkClearDepthStencilValue value;
    };

    class ClearValue {
    public:
        enum Type {
            none,
            color,
            depth
        };

        constexpr ClearValue()                 noexcept : _color(),      _type(none) {}
        constexpr ClearValue(ClearColor clear) noexcept : _color(clear), _type(color) {}
        constexpr ClearValue(ClearDepth clear) noexcept : _depth(clear), _type(depth) {}

        qz_nodiscard constexpr VkClearValue value() const noexcept {
            switch (_type) {
                case none:  return {};
                case color: return { .color = _color.value };
                case depth: return { .depthStencil = _depth.value };
            }
            qz_unreachable();
        }

        qz_nodiscard constexpr Type type() const noexcept {
            return _type;
        }

    private:
        union {
            ClearColor _color;
            ClearDepth _depth;
        };
        Type _type;
    };
} // namespace qz::gfx