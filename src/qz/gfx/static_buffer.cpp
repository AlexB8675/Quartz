#include <qz/gfx/static_buffer.hpp>
#include <qz/gfx/context.hpp>

namespace qz::gfx {
    qz_nodiscard StaticBuffer StaticBuffer::create(const Context& context, CreateInfo&& info) noexcept {
        VkBufferCreateInfo buffer_create_info{};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.flags = {};
        buffer_create_info.size = info.capacity;
        buffer_create_info.usage = info.flags;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = nullptr;

        VmaAllocationCreateInfo allocation_create_info{};
        allocation_create_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        allocation_create_info.requiredFlags = {};
        allocation_create_info.preferredFlags = {};
        allocation_create_info.memoryTypeBits = {};
        allocation_create_info.pool = nullptr;
        allocation_create_info.pUserData = nullptr;
        allocation_create_info.usage = info.usage;

        StaticBuffer buffer{};
        VmaAllocationInfo allocation_info{};
        qz_vulkan_check(vmaCreateBuffer(
            context.allocator,
            &buffer_create_info,
            &allocation_create_info,
            &buffer.handle,
            &buffer.allocation,
            &allocation_info));
        buffer.flags = info.flags;
        buffer.capacity = info.capacity;
        buffer.mapped = allocation_info.pMappedData;
        return buffer;
    }

    void StaticBuffer::destroy(const Context& context, StaticBuffer& buffer) noexcept {
        vmaDestroyBuffer(context.allocator, buffer.handle, buffer.allocation);
        buffer = {};
    }
} // namespace qz::gfx
