#include <qz/gfx/static_texture.hpp>
#include <qz/gfx/static_mesh.hpp>
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
    qz_nodiscard gfx::StaticMesh from_handle(meta::Handle<gfx::StaticMesh> handle) noexcept {
        std::lock_guard<std::mutex> lock(mutex<gfx::StaticMesh>);
        return assets<gfx::StaticMesh>[handle.index].first;
    }

    template <>
    qz_nodiscard bool is_ready(meta::Handle<gfx::StaticMesh> handle) noexcept {
        std::lock_guard<std::mutex> lock(mutex<gfx::StaticMesh>);
        return assets<gfx::StaticMesh>[handle.index].second;
    }

    template <>
    void finalize(meta::Handle<gfx::StaticMesh> handle, gfx::StaticMesh&& data) noexcept {
        std::lock_guard<std::mutex> lock(mutex<gfx::StaticMesh>);
        assets<gfx::StaticMesh>[handle.index] = {
            data, true
        };
    }


    template <>
    qz_nodiscard meta::Handle<gfx::StaticTexture> emplace_empty() noexcept {
        std::lock_guard<std::mutex> lock(mutex<gfx::StaticTexture>);
        assets<gfx::StaticTexture>.emplace_back();
        return { assets<gfx::StaticTexture>.size() - 1 };
    }

    template <>
    qz_nodiscard gfx::StaticTexture from_handle(meta::Handle<gfx::StaticTexture> handle) noexcept {
        std::lock_guard<std::mutex> lock(mutex<gfx::StaticTexture>);
        return assets<gfx::StaticTexture>[handle.index].first;
    }

    template <>
    qz_nodiscard bool is_ready(meta::Handle<gfx::StaticTexture> handle) noexcept {
        std::lock_guard<std::mutex> lock(mutex<gfx::StaticTexture>);
        return assets<gfx::StaticTexture>[handle.index].second;
    }

    template <>
    void finalize(meta::Handle<gfx::StaticTexture> handle, gfx::StaticTexture&& data) noexcept {
        std::lock_guard<std::mutex> lock(mutex<gfx::StaticTexture>);
        assets<gfx::StaticTexture>[handle.index] = {
            data, true
        };
    }

    void free_all_resources(const gfx::Context& context) noexcept {
        for (auto& [mesh, _] : assets<gfx::StaticMesh>) {
            gfx::StaticMesh::destroy(context, mesh);
        }
        assets<gfx::StaticMesh>.clear();

        for (auto& [texture, _] : assets<gfx::StaticTexture>) {
            gfx::StaticTexture::destroy(context, texture);
        }
        assets<gfx::StaticTexture>.clear();
    }

    gfx::StaticTexture& default_texture() noexcept {
        std::lock_guard<std::mutex> lock(mutex<gfx::StaticTexture>);
        return assets<gfx::StaticTexture>[0].first;
    }
} // namespace qz::assets