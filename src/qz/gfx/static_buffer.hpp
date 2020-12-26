#pragma once

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace qz::gfx {
    struct StaticBuffer {
        struct CreateInfo {
            VkBufferUsageFlags flags;
            VmaMemoryUsage usage;
            std::size_t capacity;
        };

        VmaAllocation allocation;
        std::size_t capacity;
        VkBuffer handle;
        void* mapped;

        qz_nodiscard static StaticBuffer create(const Context&, CreateInfo&&) noexcept;
        static void destroy(const Context&, StaticBuffer&) noexcept;
    };
} // namespace qz::gfx