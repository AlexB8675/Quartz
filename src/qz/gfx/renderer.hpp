#pragma once

#include <qz/gfx/command_buffer.hpp>
#include <qz/gfx/swapchain.hpp>
#include <qz/gfx/pipeline.hpp>
#include <qz/meta/types.hpp>

#include <qz/util/macros.hpp>
#include <qz/util/hash.hpp>
#include <qz/util/fwd.hpp>

#include <unordered_map>
#include <vector>
#include <mutex>

#include <vulkan/vulkan.h>

namespace qz::gfx {
    struct FrameInfo {
        std::uint32_t index;
        std::uint32_t image_idx;
        VkSemaphore img_ready;
        VkSemaphore gfx_done;
        VkFence cmd_wait;
        const Image* image;
    };

    struct Renderer {
        Swapchain swapchain;

        std::uint32_t image_idx;
        std::uint32_t frame_idx;

        meta::in_flight_array_t<CommandBuffer> gfx_cmds;
        meta::in_flight_array_t<VkSemaphore> img_ready;
        meta::in_flight_array_t<VkSemaphore> gfx_done;
        meta::in_flight_array_t<VkFence> cmd_wait;

        std::unordered_map<descriptor_layout_t, VkDescriptorSetLayout> layout_cache;

        qz_nodiscard static Renderer create(const Context&, const Window&) noexcept;
        static void destroy(const Context&, Renderer&) noexcept;
    };

    qz_nodiscard std::pair<CommandBuffer, FrameInfo> acquire_next_frame(Renderer&, const Context&) noexcept;
    void present_frame(Renderer&, const Context&, const CommandBuffer&, const FrameInfo&, VkPipelineStageFlags) noexcept;
} // namespace qz::gfx