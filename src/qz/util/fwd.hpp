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
} // namespace qz::gfx

namespace qz::meta {
    template <typename>
    struct Handle;
}