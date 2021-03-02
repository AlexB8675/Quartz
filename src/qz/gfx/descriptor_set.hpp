#pragma once

#include <qz/meta/constants.hpp>
#include <qz/meta/types.hpp>

#include <qz/util/macros.hpp>
#include <qz/util/hash.hpp>
#include <qz/util/fwd.hpp>

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <variant>

namespace qz::gfx {
    template <std::size_t = meta::in_flight>
    class DescriptorSet;

    template <>
    class DescriptorSet<1> {
        using BoundDescriptors =
            std::unordered_map<DescriptorBinding,
                std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo, std::size_t>>;
        VkDescriptorSet _handle;
        BoundDescriptors _bound;
    public:
        qz_nodiscard static DescriptorSet<1> from_raw(VkDescriptorSet) noexcept;
        qz_nodiscard static DescriptorSet<1> allocate(const Context&, const DescriptorSetLayout&) noexcept;
        static void destroy(const Context&, DescriptorSet<1>&) noexcept;

        static void bind(const Context&, DescriptorSet<1>&, const DescriptorBinding&, const Buffer<1>&) noexcept;
        static void bind(const Context&, DescriptorSet<1>&, const DescriptorBinding&, meta::Handle<StaticTexture>) noexcept;
        static void bind(const Context&, DescriptorSet<1>&, const DescriptorBinding&, const std::vector<VkDescriptorImageInfo>&) noexcept;
        qz_nodiscard VkDescriptorSet handle() const noexcept;
        qz_nodiscard const VkDescriptorSet* ptr_handle() const noexcept;
    };

    template <>
    class DescriptorSet<> {
        meta::in_flight_array_t<DescriptorSet<1>> _handles;
    public:
        qz_nodiscard static DescriptorSet<> allocate(const Context&, const DescriptorSetLayout&) noexcept;
        static void destroy(const Context&, DescriptorSet<>&) noexcept;

        qz_nodiscard DescriptorSet<1>& operator [](std::size_t) noexcept;
        qz_nodiscard const DescriptorSet<1>& operator [](std::size_t) const noexcept;
    };
} // namespace qz::gfx