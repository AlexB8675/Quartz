#pragma once

struct GLFWwindow; // Ugly thing.
namespace qz::gfx {
    struct Window;
    struct Context;
    struct Renderer;

    struct Image;
    struct Swapchain;
    class RenderPass;
    struct Pipeline;
    class CommandBuffer;
    struct StaticBuffer;
    struct StaticMesh;
    class Queue;
    struct DescriptorBinding;
    struct DescriptorImageInfo;
    template <size_t>
    class DescriptorSet;
    template <size_t>
    class Buffer;
    class StaticTexture;
} // namespace qz::gfx

namespace qz::meta {
    template <typename>
    struct Handle;
}