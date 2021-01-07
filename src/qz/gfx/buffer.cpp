#include <qz/gfx/buffer.hpp>

namespace qz::gfx {
    qz_nodiscard Buffer<1> Buffer<1>::from_raw(StaticBuffer&& handle) noexcept {
        Buffer buffer{};
        buffer._buffer = handle;
        return buffer;
    }

    qz_nodiscard Buffer<1> Buffer<1>::allocate(const Context& context, std::size_t size, meta::uniform_buffer_tag_t) noexcept {
        return from_raw(StaticBuffer::create(context, {
            .flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
            .capacity = size,
        }));
    }

    qz_nodiscard Buffer<1> Buffer<1>::allocate(const Context& context, std::size_t size, meta::storage_buffer_tag_t) noexcept {
        return from_raw(StaticBuffer::create(context, {
            .flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
            .capacity = size,
        }));
    }

    void Buffer<1>::destroy(const Context& context, Buffer<1>& buffer) noexcept {
        StaticBuffer::destroy(context, buffer._buffer);
        buffer = {};
    }

    void Buffer<1>::write(const void* data, std::size_t size) noexcept {
        std::memcpy(_buffer.mapped, data, size);
    }

    void Buffer<1>::write(const void* data, meta::whole_size_tag_t) noexcept {
        write(data, _buffer.capacity);
    }

    qz_nodiscard const void* Buffer<1>::view() const noexcept {
        return _buffer.mapped;
    }

    qz_nodiscard VkDescriptorBufferInfo Buffer<1>::info() const noexcept {
        return {
            _buffer.handle, 0, _buffer.capacity
        };
    }

    qz_nodiscard Buffer<> Buffer<>::allocate(const Context& context, std::size_t size, meta::uniform_buffer_tag_t) noexcept {
        Buffer buffer{};
        for (auto& each : buffer._handles) {
            each = Buffer<1>::allocate(context, size, meta::uniform_buffer);
        }
        return buffer;
    }

    qz_nodiscard Buffer<> Buffer<>::allocate(const Context& context, std::size_t size, meta::storage_buffer_tag_t) noexcept {
        Buffer buffer{};
        for (auto& each : buffer._handles) {
            each = Buffer<1>::allocate(context, size, meta::storage_buffer);
        }
        return buffer;
    }

    void Buffer<>::destroy(const Context& context, Buffer<>& buffer) noexcept {
        for (auto& each : buffer._handles) {
            Buffer<1>::destroy(context, each);
        }
        buffer = {};
    }

    void Buffer<>::write(const void* data, std::size_t size) noexcept {
        for (auto& each : _handles) {
            each.write(data, size);
        }
    }

    void Buffer<>::write(const void* data, meta::whole_size_tag_t tag) noexcept {
        for (auto& each : _handles) {
            each.write(data, tag);
        }
    }

    qz_nodiscard Buffer<1>& Buffer<>::operator [](std::size_t index) noexcept {
        return _handles[index];
    }

    qz_nodiscard const Buffer<1>& Buffer<>::operator [](std::size_t index) const noexcept {
        return _handles[index];
    }
} // namespace qz::gfx