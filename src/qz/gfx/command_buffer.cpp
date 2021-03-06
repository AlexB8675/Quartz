#include <qz/gfx/descriptor_set.hpp>
#include <qz/gfx/command_buffer.hpp>
#include <qz/gfx/static_buffer.hpp>
#include <qz/gfx/static_mesh.hpp>
#include <qz/gfx/render_pass.hpp>
#include <qz/gfx/pipeline.hpp>
#include <qz/gfx/context.hpp>
#include <qz/gfx/assets.hpp>
#include <qz/gfx/queue.hpp>

namespace qz::gfx {
    qz_nodiscard CommandBuffer CommandBuffer::from_raw(VkCommandPool pool, VkCommandBuffer handle) noexcept {
        CommandBuffer result{};
        result._handle = handle;
        result._pool = pool;
        return result;
    }

    qz_nodiscard CommandBuffer CommandBuffer::allocate(const Context& context, VkCommandPool pool) noexcept {
        VkCommandBuffer handle;
        VkCommandBufferAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = pool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;
        qz_vulkan_check(vkAllocateCommandBuffers(context.device, &allocate_info, &handle));

        return from_raw(pool, handle);
    }

    void CommandBuffer::destroy(const Context& context, CommandBuffer& handle) noexcept {
        vkFreeCommandBuffers(context.device, handle._pool, 1, &handle._handle);
        handle = {};
    }

    CommandBuffer& CommandBuffer::begin() noexcept {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        qz_vulkan_check(vkBeginCommandBuffer(_handle, &begin_info));
        return *this;
    }

    CommandBuffer& CommandBuffer::begin_render_pass(const RenderPass& render_pass, std::size_t framebuffer) noexcept {
        _active_pass = &render_pass;
        const auto clear_values = render_pass.clears();

        VkRenderPassBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begin_info.renderPass = render_pass.handle();
        begin_info.framebuffer = render_pass.framebuffer(framebuffer);
        begin_info.renderArea.extent = render_pass.extent();
        begin_info.clearValueCount = clear_values.size();
        begin_info.pClearValues = clear_values.data();
        vkCmdBeginRenderPass(_handle, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
        return *this;
    }

    CommandBuffer& CommandBuffer::set_viewport(meta::viewport_tag_t) noexcept {
        const auto extent = _active_pass->extent();
        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = extent.height;
        viewport.width = extent.width;
        viewport.height = -(int32_t)extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        set_viewport(viewport);
        return *this;
    }

    CommandBuffer& CommandBuffer::set_viewport(VkViewport viewport) noexcept {
        vkCmdSetViewport(_handle, 0, 1, &viewport);
        return *this;
    }

    CommandBuffer& CommandBuffer::set_scissor(meta::scissor_tag_t) noexcept {
        VkRect2D scissor{ {}, _active_pass->extent() };
        set_scissor(scissor);
        return *this;
    }

    CommandBuffer& CommandBuffer::set_scissor(VkRect2D scissor) noexcept {
        vkCmdSetScissor(_handle, 0, 1, &scissor);
        return *this;
    }

    CommandBuffer& CommandBuffer::bind_pipeline(const Pipeline& pipeline) noexcept {
        vkCmdBindPipeline(_handle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle());
        _active_pipeline = &pipeline;
        return *this;
    }

    CommandBuffer& CommandBuffer::bind_descriptor_set(const DescriptorSet<1>& set) noexcept {
        vkCmdBindDescriptorSets(_handle, VK_PIPELINE_BIND_POINT_GRAPHICS, _active_pipeline->layout(), 0, 1, set.ptr_handle(), 0, nullptr);
        return *this;
    }

    CommandBuffer& CommandBuffer::bind_vertex_buffer(const StaticBuffer& vertex) noexcept {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(_handle, 0, 1, &vertex.handle, &offset);
        return *this;
    }

    CommandBuffer& CommandBuffer::bind_index_buffer(const StaticBuffer& index) noexcept {
        vkCmdBindIndexBuffer(_handle, index.handle, 0, VK_INDEX_TYPE_UINT32);
        return *this;
    }

    CommandBuffer& CommandBuffer::bind_static_mesh(meta::Handle<StaticMesh> handle) noexcept {
        _ready = false;
        const auto lock = assets::acquire<StaticMesh>();
        qz_likely_if(assets::is_ready(handle)) {
            const auto mesh = assets::from_handle(handle);
            bind_vertex_buffer(mesh.geometry);
            bind_index_buffer(mesh.indices);
            _ready = true;
        }
        return *this;
    }

    CommandBuffer& CommandBuffer::push_constants(VkPipelineStageFlags flags, std::size_t size, const void* data) noexcept {
        vkCmdPushConstants(_handle, _active_pipeline->layout(), flags, 0, size, data);
        return *this;
    }

    CommandBuffer& CommandBuffer::draw(std::uint32_t vertices,
                                       std::uint32_t instances,
                                       std::uint32_t first_vertex,
                                       std::uint32_t first_instance) noexcept {
        qz_likely_if(_ready) {
            vkCmdDraw(_handle, vertices, instances, first_vertex, first_instance);
        }
        return *this;
    }

    CommandBuffer& CommandBuffer::draw_indexed(std::uint32_t indices,
                                               std::uint32_t instances,
                                               std::uint32_t first_index,
                                               std::uint32_t first_instance) noexcept {
        qz_likely_if(_ready) {
            vkCmdDrawIndexed(_handle, indices, instances, first_index, 0, first_instance);
        }
        return *this;
    }

    CommandBuffer& CommandBuffer::end_render_pass() noexcept {
        _active_pipeline = nullptr;
        _active_pass = nullptr;
        vkCmdEndRenderPass(_handle);
        return *this;
    }

    CommandBuffer& CommandBuffer::copy_image(const Image& source, const Image& dest) noexcept {
        VkImageCopy region{};
        region.srcSubresource = {
            .aspectMask = source.aspect,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        };
        region.srcOffset = {};
        region.dstSubresource = {
            .aspectMask = dest.aspect,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        };
        region.dstOffset = {};
        region.extent = { source.width, source.height, 1 };
        vkCmdCopyImage(_handle,
            source.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dest.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region);
        return *this;
    }

    CommandBuffer& CommandBuffer::blit_image(const ImageBlit& info) noexcept {
        const auto& source = *info.source_image;
        const auto& dest   = info.dest_image ? *info.dest_image : source;
        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = source.aspect;
        blit.srcSubresource.mipLevel = info.source_mip;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[1].x = info.source_off.x ? info.source_off.x : 1;
        blit.srcOffsets[1].y = info.source_off.y ? info.source_off.y : 1;
        blit.srcOffsets[1].z = info.source_off.z ? info.source_off.z : 1;
        blit.dstSubresource.aspectMask = dest.aspect;
        blit.dstSubresource.mipLevel = info.dest_mip;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        blit.dstOffsets[1].x = info.dest_off.x ? info.dest_off.x : 1;
        blit.dstOffsets[1].y = info.dest_off.y ? info.dest_off.y : 1;
        blit.dstOffsets[1].z = info.dest_off.z ? info.dest_off.z : 1;
        vkCmdBlitImage(_handle,
            source.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dest.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);
        return *this;
    }

    CommandBuffer& CommandBuffer::copy_buffer(const StaticBuffer& source, const StaticBuffer& dest) noexcept {
        VkBufferCopy region{};
        region.size = source.capacity;
        region.srcOffset = 0;
        region.dstOffset = 0;
        vkCmdCopyBuffer(_handle, source.handle, dest.handle, 1, &region);
        return *this;
    }

    CommandBuffer& CommandBuffer::copy_buffer_to_image(const StaticBuffer& source, const Image& dest) noexcept {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = dest.aspect;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { dest.width, dest.height, 1 };
        vkCmdCopyBufferToImage(_handle, source.handle, dest.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        return *this;
    }

    CommandBuffer& CommandBuffer::transfer_ownership(const BufferMemoryBarrier& info, const Queue& source, const Queue& dest) noexcept {
        VkBufferMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = info.source_access;
        barrier.dstAccessMask = info.dest_access;
        barrier.srcQueueFamilyIndex = source.family();
        barrier.dstQueueFamilyIndex = dest.family();
        barrier.buffer = info.buffer->handle;
        barrier.offset = 0;
        barrier.size = info.buffer->capacity;
        vkCmdPipelineBarrier(
            _handle,
            info.source_stage,
            info.dest_stage,
            VkDependencyFlags{},
            0, nullptr,
            1, &barrier,
            0, nullptr);
        return *this;
    }

    CommandBuffer& CommandBuffer::transfer_ownership(const ImageMemoryBarrier& info, const Queue& source, const Queue& dest) noexcept {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = info.source_access;
        barrier.dstAccessMask = info.dest_access;
        barrier.oldLayout = info.old_layout;
        barrier.newLayout = info.new_layout;
        barrier.srcQueueFamilyIndex = source.family();
        barrier.dstQueueFamilyIndex = dest.family();
        barrier.image = info.image->handle;
        barrier.subresourceRange = {
            .aspectMask = info.image->aspect,
            .baseMipLevel = 0,
            .levelCount = info.image->mips,
            .baseArrayLayer = 0,
            .layerCount = 1
        };
        vkCmdPipelineBarrier(
            _handle,
            info.source_stage,
            info.dest_stage,
            VkDependencyFlags{},
            0, nullptr,
            0, nullptr,
            1, &barrier);
        return *this;
    }

    CommandBuffer& CommandBuffer::insert_layout_transition(const ImageMemoryBarrier& info) noexcept {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = info.source_access;
        barrier.dstAccessMask = info.dest_access;
        barrier.oldLayout = info.old_layout;
        barrier.newLayout = info.new_layout;
        barrier.srcQueueFamilyIndex = meta::family_ignored;
        barrier.dstQueueFamilyIndex = meta::family_ignored;
        barrier.image = info.image->handle;
        barrier.subresourceRange = {
            .aspectMask = info.image->aspect,
            .baseMipLevel = info.mip,
            .levelCount = info.levels == 0 ? info.image->mips : info.levels,
            .baseArrayLayer = 0,
            .layerCount = 1
        };
        vkCmdPipelineBarrier(
            _handle,
            info.source_stage,
            info.dest_stage,
            VkDependencyFlags{},
            0, nullptr,
            0, nullptr,
            1, &barrier);
        return *this;
    }

    void CommandBuffer::end() noexcept {
        qz_vulkan_check(vkEndCommandBuffer(_handle));
    }

    qz_nodiscard VkCommandBuffer CommandBuffer::handle() const noexcept {
        return _handle;
    }

    qz_nodiscard const VkCommandBuffer* CommandBuffer::ptr_handle() const noexcept {
        return &_handle;
    }
} // namespace qz::gfx