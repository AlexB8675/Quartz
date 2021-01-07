#pragma once

#include <qz/gfx/static_buffer.hpp>

#include <qz/meta/constants.hpp>
#include <qz/meta/types.hpp>

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <vulkan/vulkan.h>

namespace qz::gfx {
    template <std::size_t extent = meta::in_flight>
    class Buffer;

    template <>
    class Buffer<1> {
        StaticBuffer _buffer;
    public:
        qz_nodiscard static Buffer<1> from_raw(StaticBuffer&&) noexcept;
        qz_nodiscard static Buffer<1> allocate(const Context&, std::size_t, meta::uniform_buffer_tag_t) noexcept;
        qz_nodiscard static Buffer<1> allocate(const Context&, std::size_t, meta::storage_buffer_tag_t) noexcept;
        static void destroy(const Context&, Buffer<1>&) noexcept;

        void write(const void*, std::size_t) noexcept;
        void write(const void*, meta::whole_size_tag_t) noexcept;
        qz_nodiscard const void* view() const noexcept;
        qz_nodiscard VkDescriptorBufferInfo info() const noexcept;
    };

    template <>
    class Buffer<> {
        meta::in_flight_array<Buffer<1>> _handles;
    public:
        qz_nodiscard static Buffer<> allocate(const Context&, std::size_t, meta::uniform_buffer_tag_t) noexcept;
        qz_nodiscard static Buffer<> allocate(const Context&, std::size_t, meta::storage_buffer_tag_t) noexcept;
        static void destroy(const Context&, Buffer<>&) noexcept;

        void write(const void*, std::size_t) noexcept;
        void write(const void*, meta::whole_size_tag_t) noexcept;
        qz_nodiscard Buffer<1>& operator [](std::size_t) noexcept;
        qz_nodiscard const Buffer<1>& operator [](std::size_t) const noexcept;
    };
} // namespace qz::gfx