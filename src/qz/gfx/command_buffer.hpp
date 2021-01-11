#pragma once

#include <qz/meta/constants.hpp>
#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <qz/gfx/image.hpp>

#include <vulkan/vulkan.h>

namespace qz::gfx {
    struct BufferMemoryBarrier {
        const StaticBuffer* buffer;
        VkPipelineStageFlags source_stage;
        VkPipelineStageFlags dest_stage;
        VkAccessFlags source_access;
        VkAccessFlags dest_access;
    };

    struct ImageMemoryBarrier {
        const Image* image;
        VkPipelineStageFlags source_stage;
        VkPipelineStageFlags dest_stage;
        VkAccessFlags source_access;
        VkAccessFlags dest_access;
        VkImageLayout old_layout;
        VkImageLayout new_layout;
    };

    class CommandBuffer {
        const RenderPass* _active_pass;
        const Pipeline* _active_pipeline;
        VkCommandBuffer _handle;
        VkCommandPool _pool;
    public:
        CommandBuffer() noexcept = default;

        qz_nodiscard static CommandBuffer from_raw(VkCommandPool, VkCommandBuffer) noexcept;
        qz_nodiscard static CommandBuffer allocate(const Context&, VkCommandPool) noexcept;
        static void destroy(const Context&, CommandBuffer&) noexcept;

        qz_nodiscard VkCommandBuffer handle() const noexcept;
        qz_nodiscard VkCommandBuffer* ptr_handle() noexcept;
        qz_nodiscard const VkCommandBuffer* ptr_handle() const noexcept;

        CommandBuffer& begin() noexcept;
        CommandBuffer& begin_render_pass(const RenderPass&, std::size_t) noexcept;
        CommandBuffer& set_viewport(meta::viewport_tag_t) noexcept;
        CommandBuffer& set_viewport(VkViewport) noexcept;
        CommandBuffer& set_scissor(meta::scissor_tag_t) noexcept;
        CommandBuffer& set_scissor(VkRect2D) noexcept;
        CommandBuffer& bind_pipeline(const Pipeline&) noexcept;
        CommandBuffer& bind_descriptor_set(const DescriptorSet<1>&) noexcept;
        CommandBuffer& bind_push_constants(VkPipelineStageFlags, std::size_t, const void*) noexcept;
        CommandBuffer& bind_vertex_buffer(const StaticBuffer&) noexcept;
        CommandBuffer& bind_index_buffer(const StaticBuffer&) noexcept;
        CommandBuffer& bind_static_mesh(const StaticMesh&) noexcept;
        CommandBuffer& draw(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) noexcept;
        CommandBuffer& draw_indexed(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) noexcept;
        CommandBuffer& end_render_pass() noexcept;
        CommandBuffer& copy_image(const Image&, const Image&) noexcept;
        CommandBuffer& copy_buffer(const StaticBuffer&, const StaticBuffer&) noexcept;
        CommandBuffer& copy_buffer_to_image(const StaticBuffer&, const Image&) noexcept;
        CommandBuffer& transfer_ownership(const BufferMemoryBarrier&, const Queue&, const Queue&) noexcept;
        CommandBuffer& transfer_ownership(const ImageMemoryBarrier&, const Queue&, const Queue&) noexcept;
        CommandBuffer& insert_layout_transition(const ImageMemoryBarrier&) noexcept;
        void end() noexcept;
    };
} // namespace qz::gfx