#include <qz/gfx/descriptor_set.hpp>
#include <qz/gfx/pipeline.hpp>
#include <qz/gfx/context.hpp>
#include <qz/gfx/buffer.hpp>

namespace qz::gfx {
    qz_nodiscard static bool compare_descriptors(VkDescriptorBufferInfo lhs, VkDescriptorBufferInfo rhs) noexcept {
        return lhs.buffer == rhs.buffer &&
               lhs.offset == rhs.offset &&
               lhs.range  == rhs.range;
    }

    qz_nodiscard DescriptorSet<1> DescriptorSet<1>::from_raw(VkDescriptorSet handle) noexcept {
        DescriptorSet<1> set{};
        set._handle = handle;
        return set;
    }

    qz_nodiscard DescriptorSet<1> DescriptorSet<1>::allocate(const Context& context, VkDescriptorSetLayout layout) noexcept {
        VkDescriptorSetAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate_info.descriptorPool = context.descriptor_pool;
        allocate_info.descriptorSetCount = 1;
        allocate_info.pSetLayouts = &layout;

        VkDescriptorSet set;
        qz_vulkan_check(vkAllocateDescriptorSets(context.device, &allocate_info, &set));
        return from_raw(set);
    }

    void DescriptorSet<1>::destroy(const Context& context, DescriptorSet<1>& set) noexcept {
        qz_vulkan_check(vkFreeDescriptorSets(context.device, context.descriptor_pool, 1, &set._handle));
        set = {};
    }

    void DescriptorSet<1>::bind(const Context& context, DescriptorSet<1>& set, const DescriptorBinding& binding, const Buffer<1>& buffer) noexcept {
        const auto descriptor = buffer.info();
        if (!compare_descriptors(set._bound[binding], descriptor)) {
            VkWriteDescriptorSet update{};
            update.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            update.pNext = nullptr;
            update.dstSet = set._handle;
            update.dstBinding = binding.index;
            update.dstArrayElement = 0;
            update.descriptorCount = 1;
            update.descriptorType = binding.type;
            update.pImageInfo = nullptr;
            update.pBufferInfo = &descriptor;
            update.pTexelBufferView = nullptr;
            vkUpdateDescriptorSets(context.device, 1, &update, 0, nullptr);
            set._bound[binding] = descriptor;
        }
    }

    qz_nodiscard VkDescriptorSet DescriptorSet<1>::handle() const noexcept {
        return _handle;
    }

    qz_nodiscard const VkDescriptorSet* DescriptorSet<1>::ptr_handle() const noexcept {
        return &_handle;
    }

    qz_nodiscard DescriptorSet<> DescriptorSet<>::allocate(const Context& context, VkDescriptorSetLayout layout) noexcept {
        DescriptorSet<> sets{};
        for (auto& each : sets._handles) {
            each = DescriptorSet<1>::allocate(context, layout);
        }
        return sets;
    }

    void DescriptorSet<>::destroy(const Context& context, DescriptorSet<>& set) noexcept {
        for (auto& each : set._handles) {
            DescriptorSet<1>::destroy(context, each);
        }
        set = {};
    }

    qz_nodiscard DescriptorSet<1>& DescriptorSet<>::operator [](std::size_t index) noexcept {
        return _handles[index];
    }

    qz_nodiscard const DescriptorSet<1>& DescriptorSet<>::operator [](std::size_t index) const noexcept {
        return _handles[index];
    }
} // namespace qz::gfx