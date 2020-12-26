#include <qz/gfx/static_mesh.hpp>
#include <qz/gfx/static_buffer.hpp>
#include <qz/gfx/assets.hpp>

#include <algorithm>
#include <vector>
#include <mutex>

namespace qz::assets {
    template <typename T>
    using AssetStorage = std::pair<T, bool>;

    template <typename T>
    static std::vector<AssetStorage<T>> assets;

    template <typename T>
    static std::mutex mutex;

    template <>
    qz_nodiscard meta::Handle<gfx::StaticMesh> emplace_empty() noexcept {
        std::lock_guard<std::mutex> lock(mutex<gfx::StaticMesh>);
        assets<gfx::StaticMesh>.emplace_back();
        return { assets<gfx::StaticMesh>.size() - 1 };
    }

    template <>
    qz_nodiscard gfx::StaticMesh& from_handle(meta::Handle<gfx::StaticMesh> handle) noexcept {
        std::lock_guard<std::mutex> lock(mutex<gfx::StaticMesh>);
        qz_assert(0 <= handle.index && handle.index < assets<gfx::StaticMesh>.size(), "Invalid mesh handle");
        return assets<gfx::StaticMesh>[handle.index].first;
    }

    template <>
    qz_nodiscard bool is_ready(meta::Handle<gfx::StaticMesh> handle) noexcept {
        std::lock_guard<std::mutex> lock(mutex<gfx::StaticMesh>);
        qz_assert(0 <= handle.index && handle.index < assets<gfx::StaticMesh>.size(), "Invalid mesh handle");
        return assets<gfx::StaticMesh>[handle.index].second;
    }

    template <>
    void finalize(meta::Handle<gfx::StaticMesh> handle, gfx::StaticMesh&& data) noexcept {
        std::lock_guard<std::mutex> lock(mutex<gfx::StaticMesh>);
        qz_assert(0 <= handle.index && handle.index < assets<gfx::StaticMesh>.size(), "Invalid mesh handle");
        assets<gfx::StaticMesh>[handle.index] = {
            data, true
        };
    }

    template <>
    void wait_all<gfx::StaticMesh>() noexcept {
        while (!std::all_of(assets<gfx::StaticMesh>.begin(), assets<gfx::StaticMesh>.end(), [](const auto& each) {
            return each.second;
        }));
    }

    void free_all_resources(const gfx::Context& context) noexcept {
        for (auto& [data, _] : assets<gfx::StaticMesh>) {
            gfx::StaticBuffer::destroy(context, data.geometry);
            gfx::StaticBuffer::destroy(context, data.indices);
        }
        assets<gfx::StaticMesh>.clear();
    }
} // namespace qz::assets