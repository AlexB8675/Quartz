#pragma once

namespace qz::meta {
    constexpr struct viewport_tag_t {} full_viewport;
    constexpr struct scissor_tag_t {} full_scissor;

    constexpr struct uniform_buffer_tag_t{} uniform_buffer;
    constexpr struct storage_buffer_tag_t{} storage_buffer;

    constexpr struct whole_size_tag_t{} whole_size;

    constexpr auto in_flight = 2u;
    constexpr auto external_subpass = ~0u;
    constexpr auto family_ignored = ~0u;
} // namespace qz::meta