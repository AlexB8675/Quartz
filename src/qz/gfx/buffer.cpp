#include <qz/gfx/buffer.hpp>

#include <cstring>
#include <string>

namespace qz::gfx {
    qz_nodiscard Buffer<1> Buffer<1>::from_raw(StaticBuffer&& handle, std::size_t size) noexcept {
        Buffer buffer{};
        buffer._handle = handle;
        buffer._size = size;
        return buffer;
    }

    qz_nodiscard Buffer<1> Buffer<1>::allocate(const Context& context, std::size_t size, meta::BufferKind kind) noexcept {
        return from_raw(StaticBuffer::create(context, {
            .flags = [kind]() -> VkBufferUsageFlags {
                switch (kind) {
                    case meta::uniform_buffer: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                    case meta::storage_buffer: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                }
                qz_unreachable();
            }(),
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
            .capacity = size,
        }), size);
    }

    void Buffer<1>::resize(const Context& context, Buffer<1>& buffer, std::size_t size) noexcept {
        if (size > buffer.capacity()) {
            const auto flags = buffer._handle.flags;
            std::string temp(size, '\0');
            std::memcpy(temp.data(), buffer.view(), size);
            StaticBuffer::destroy(context, buffer._handle);
            buffer._handle = StaticBuffer::create(context, {
                .flags = flags,
                .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                .capacity = size
            });
            buffer.write(temp.data(), size);
        }
    }

    void Buffer<1>::destroy(const Context& context, Buffer<1>& buffer) noexcept {
        StaticBuffer::destroy(context, buffer._handle);
        buffer = {};
    }

    void Buffer<1>::write(const void* data, std::size_t size, std::size_t offset) noexcept {
        qz_assert(size + offset <= _handle.capacity, "can't write past end pointer");
        _size = std::max(_size, size + offset);
        std::memcpy(static_cast<char*>(_handle.mapped) + offset, data, size);
    }

    void Buffer<1>::write(const void* data, meta::whole_size_tag_t) noexcept {
        std::memcpy(_handle.mapped, data, _handle.capacity);
    }

    qz_nodiscard void* Buffer<1>::view() noexcept {
        return _handle.mapped;
    }

    qz_nodiscard const void* Buffer<1>::view() const noexcept {
        return _handle.mapped;
    }

    qz_nodiscard VkDescriptorBufferInfo Buffer<1>::info() const noexcept {
        return {
            _handle.handle, 0, _size
        };
    }

    qz_nodiscard std::size_t Buffer<1>::size() const noexcept {
        return _size;
    }

    qz_nodiscard std::size_t Buffer<1>::capacity() const noexcept {
        return _handle.capacity;
    }

    qz_nodiscard Buffer<> Buffer<>::allocate(const Context& context, std::size_t size, meta::BufferKind kind) noexcept {
        Buffer buffer{};
        for (auto& each : buffer._handles) {
            each = Buffer<1>::allocate(context, size, kind);
        }
        return buffer;
    }

    void Buffer<>::resize(const Context& context, Buffer<>& buffer, std::size_t size) noexcept {
        for (auto& each : buffer._handles) {
            Buffer<1>::resize(context, each, size);
        }
    }

    void Buffer<>::destroy(const Context& context, Buffer<>& buffer) noexcept {
        for (auto& each : buffer._handles) {
            Buffer<1>::destroy(context, each);
        }
        buffer = {};
    }

    void Buffer<>::write(const void* data, std::size_t size, std::size_t offset) noexcept {
        for (auto& each : _handles) {
            each.write(data, size, offset);
        }
    }

    void Buffer<>::write(const void* data, meta::whole_size_tag_t) noexcept {
        for (auto& each : _handles) {
            each.write(data, meta::whole_size);
        }
    }

    qz_nodiscard Buffer<1>& Buffer<>::operator [](std::size_t index) noexcept {
        return _handles[index];
    }

    qz_nodiscard const Buffer<1>& Buffer<>::operator [](std::size_t index) const noexcept {
        return _handles[index];
    }
} // namespace qz::gfx