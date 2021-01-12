#include <qz/util/file_view.hpp>

#if defined(_WIN32)
    #include <Windows.h>
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
#endif

namespace qz::util {
    qz_nodiscard FileView FileView::create(std::string_view path) noexcept {
        FileView file{};
#if defined(_WIN32)
        file._handle = CreateFile(path.data(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        file._mapping = CreateFileMapping(file._handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
        file._size = GetFileSize(file._handle, nullptr);
        file._data = MapViewOfFile(file._mapping, FILE_MAP_READ, 0, 0, file._size);
#else
        const auto file_desc = open(path.data(), O_RDONLY);
        struct stat file_info{};
        qz_assert(fstat(file_desc, &file_info) != -1, "failed to get file info");
        file._data = mmap(nullptr, file_info.st_size, PROT_READ, MAP_PRIVATE, file_desc, 0);
        file._size = file_info.st_size;
#endif
        qz_assert(file._data, "file not found");
        return file;
    }

    void FileView::destroy(FileView& file) noexcept {
#if defined(_WIN32)
        UnmapViewOfFile(file._data);
        CloseHandle(file._handle);
        CloseHandle(file._mapping);
#else
        munmap((void*)file._data, file._size);
#endif
        file = {};
    }

    qz_nodiscard const void* FileView::data() const noexcept {
        return _data;
    }

    qz_nodiscard std::size_t FileView::size() const noexcept {
        return _size;
    }
} // namespace qz::util
