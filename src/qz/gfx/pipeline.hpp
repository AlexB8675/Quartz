#pragma once

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <cstdint>
#include <string>
#include <vector>

namespace qz::gfx {
    enum class VertexAttribute : std::uint32_t {
        vec1 = sizeof(float[1]),
        vec2 = sizeof(float[2]),
        vec3 = sizeof(float[3]),
        vec4 = sizeof(float[4])
    };

    struct DescriptorBinding {
        bool dynamic;
        std::string name;
        std::size_t index;
        std::uint32_t count;
        VkDescriptorType type;
        VkShaderStageFlags stage;

        qz_make_equal_to(DescriptorBinding, dynamic, name, index, count, type, stage);
    };

    using descriptor_layout_t = std::vector<DescriptorBinding>;

    struct DescriptorSetLayout {
        VkDescriptorSetLayout handle;
        descriptor_layout_t descriptors;
    };

    using descriptor_set_layouts_t = std::vector<DescriptorSetLayout>;
    using descriptor_bindings_t    = std::unordered_map<std::string, DescriptorBinding>;

    struct Pipeline {
    private:
        VkPipeline _handle;
        VkPipelineLayout _layout;
        descriptor_bindings_t _bindings;
        descriptor_set_layouts_t _descriptors;
    public:
        struct CreateInfo {
            const char* vertex;
            const char* fragment;
            std::vector<VertexAttribute> attributes;
            std::vector<VkDynamicState> states;
            const RenderPass* render_pass;
            std::uint32_t subpass;
            bool depth;
        };

        qz_nodiscard static Pipeline create(const Context&, Renderer&, CreateInfo&&) noexcept;
        static void destroy(const Context&, Pipeline&) noexcept;

        qz_nodiscard VkPipeline handle() const noexcept;
        qz_nodiscard VkPipelineLayout layout() const noexcept;
        qz_nodiscard const DescriptorSetLayout& set(std::size_t) const noexcept;
        qz_nodiscard const DescriptorBinding& operator [](std::string_view) const noexcept;
    };
} // namespace qz::gfx