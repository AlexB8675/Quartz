#pragma once

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <mutex>

namespace qz::gfx {
    class Queue {
        VkQueue _handle;
        std::mutex _mutex;
        std::uint32_t _family;
    public:
        qz_nodiscard static std::unique_ptr<Queue> create(const Context&, std::uint32_t, std::uint32_t) noexcept;

        void submit(const CommandBuffer&, VkPipelineStageFlags, VkSemaphore, VkSemaphore, VkFence) noexcept;
        void present(const Swapchain&, std::uint32_t, VkSemaphore) noexcept;
        void wait_idle() const noexcept;
        qz_nodiscard std::uint32_t family() const noexcept;
    };
} // namespace qz::gfx