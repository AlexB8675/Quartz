#pragma once

#include <qz/gfx/static_buffer.hpp>

#include <qz/meta/constants.hpp>
#include <qz/meta/types.hpp>

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <vulkan/vulkan.h>

namespace qz::gfx {
    template <std::size_t = meta::in_flight>
    class Buffer;

    template <>
    class Buffer<1> {
        std::size_t _size;
        StaticBuffer _handle;
    public:
        qz_nodiscard static Buffer<1> from_raw(StaticBuffer&&, std::size_t) noexcept;
        qz_nodiscard static Buffer<1> allocate(const Context&, std::size_t, meta::BufferKind) noexcept;
        static void resize(const Context&, Buffer<1>&, std::size_t) noexcept;
        static void destroy(const Context&, Buffer<1>&) noexcept;

        void write(const void*, std::size_t, std::size_t = 0) noexcept;
        void write(const void*, meta::whole_size_tag_t) noexcept;
        qz_nodiscard void* view() noexcept;
        qz_nodiscard const void* view() const noexcept;
        qz_nodiscard VkDescriptorBufferInfo info() const noexcept;
        qz_nodiscard std::size_t size() const noexcept;
        qz_nodiscard std::size_t capacity() const noexcept;
    };

    template <>
    class Buffer<> {
        meta::in_flight_array_t<Buffer<1>> _handles;
    public:
        qz_nodiscard static Buffer<> allocate(const Context&, std::size_t, meta::BufferKind) noexcept;
        static void resize(const Context&, Buffer<>&, std::size_t) noexcept;
        static void destroy(const Context&, Buffer<>&) noexcept;

        void write(const void*, std::size_t, std::size_t = 0) noexcept;
        void write(const void*, meta::whole_size_tag_t) noexcept;
        qz_nodiscard Buffer<1>& operator [](std::size_t) noexcept;
        qz_nodiscard const Buffer<1>& operator [](std::size_t) const noexcept;
    };
} // namespace qz::gfx