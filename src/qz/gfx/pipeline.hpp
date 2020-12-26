#pragma once

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>
#include <map>

namespace qz::gfx {
    enum class VertexAttribute : std::uint32_t {
        vec1 = sizeof(float[1]),
        vec2 = sizeof(float[2]),
        vec3 = sizeof(float[3]),
        vec4 = sizeof(float[4])
    };

    struct DescriptorBinding {
        std::size_t index;
        std::uint32_t count;
        VkDescriptorType type;
        VkShaderStageFlags stage;

        qz_make_equal_to(DescriptorBinding, index, count, type, stage);
    };

    using DescriptorLayout    = std::vector<DescriptorBinding>;
    using DescriptorSetLayout = std::vector<VkDescriptorSetLayout>;

    struct Pipeline {
    private:
        VkPipeline _handle;
        VkPipelineLayout _layout;
        DescriptorSetLayout _descriptors;

    public:
        struct CreateInfo {
            const char* vertex;
            const char* fragment;
            std::vector<VertexAttribute> attributes;
            std::vector<VkDynamicState> states;
            const RenderPass* render_pass;
            std::uint32_t subpass;
        };

        qz_nodiscard static Pipeline from_raw(VkPipeline, VkPipelineLayout, DescriptorSetLayout&&) noexcept;
        qz_nodiscard static Pipeline create(const Context&, Renderer&, CreateInfo&&) noexcept;
        static void destroy(const Context&, Pipeline&) noexcept;

        qz_nodiscard VkPipeline handle() const noexcept;
        qz_nodiscard VkPipelineLayout layout() const noexcept;
        qz_nodiscard VkDescriptorSetLayout set(std::size_t) const noexcept;
    };
} // namespace qz::gfx