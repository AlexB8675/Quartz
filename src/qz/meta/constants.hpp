#pragma once

namespace qz::meta {
    constexpr struct viewport_tag_t {} full_viewport;
    constexpr struct scissor_tag_t {} full_scissor;
    constexpr struct whole_size_tag_t{} whole_size;

    enum BufferKind {
        uniform_buffer,
        storage_buffer
    };

    constexpr auto dynamic_size = 256u;
    constexpr auto in_flight = 2u;
    constexpr auto external_subpass = ~0u;
    constexpr auto family_ignored = ~0u;
    constexpr auto default_texture = 0u;
} // namespace qz::meta