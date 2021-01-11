#pragma once

#include <qz/util/macros.hpp>

#include <string_view>
#include <cstdint>

namespace qz::util {
    class FileView {
#if defined(_WIN32)
        void* _handle;
        void* _mapping;
#endif
        const std::uint8_t* _data;
        std::size_t _size;
    public:
        qz_nodiscard static FileView create(std::string_view) noexcept;
        static void destroy(FileView&) noexcept;

        qz_nodiscard const std::uint8_t* data() const noexcept;
        qz_nodiscard std::size_t size() const noexcept;
    };
} // namespace qz::util