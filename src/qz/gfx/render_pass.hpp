#pragma once

#include <qz/meta/constants.hpp>
#include <qz/gfx/clear.hpp>
#include <qz/gfx/image.hpp>

#include <vulkan/vulkan.h>

#include <type_traits>
#include <variant>
#include <vector>
#include <string>

namespace qz::gfx {
    struct Attachment {
        struct CreateInfo {
            Image image;
            std::string name;
            std::vector<std::size_t> framebuffers;
            bool owning;
            bool discard;
            VkImageLayout layout;
            ClearValue clear;
        };
        Image image;
        bool owning;
        std::string name;
        ClearValue clear;
        std::vector<std::size_t> framebuffers;
        VkAttachmentDescription description;
        VkAttachmentReference reference;
    };

    struct SubpassInfo {
        std::vector<std::string> attachments;
        std::vector<std::string> preserve;
        std::vector<std::string> input;
    };

    struct SubpassDependency {
        std::uint32_t source_subpass;
        std::uint32_t dest_subpass;
        VkPipelineStageFlags source_stage;
        VkPipelineStageFlags dest_stage;
        VkAccessFlags source_access;
        VkAccessFlags dest_access;
    };

    class RenderPass {
        VkRenderPass _handle = nullptr;
        VkPipelineStageFlags _sync_stage;
        std::vector<Attachment> _attachments;
        std::vector<VkFramebuffer> _framebuffers;
    public:
        struct CreateInfo {
            std::vector<Attachment::CreateInfo> attachments;
            std::vector<SubpassInfo> subpasses;
            std::vector<SubpassDependency> dependencies;
        };

        qz_nodiscard static RenderPass create(const Context&, CreateInfo&&) noexcept;
        static void destroy(const Context&, RenderPass&) noexcept;

        qz_nodiscard Image& image(std::string_view) noexcept;
        qz_nodiscard const Image& image(std::string_view) const noexcept;

        qz_nodiscard Attachment& attachment(std::string_view) noexcept;
        qz_nodiscard const Attachment& attachment(std::string_view) const noexcept;

        qz_nodiscard VkFramebuffer& framebuffer(std::size_t) noexcept;
        qz_nodiscard const VkFramebuffer& framebuffer(std::size_t) const noexcept;

        qz_nodiscard std::vector<VkClearValue> clears() const noexcept;
        qz_nodiscard VkExtent2D extent() const noexcept;
        qz_nodiscard VkRenderPass handle() const noexcept;
        qz_nodiscard VkPipelineStageFlags sync_stage() const noexcept;
    };
} // namespace qz::gfx
