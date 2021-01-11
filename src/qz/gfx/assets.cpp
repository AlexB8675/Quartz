#include <qz/gfx/static_texture.hpp>
#include <qz/gfx/static_mesh.hpp>
#include <qz/gfx/context.hpp>
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

    template <typename T>
    qz_nodiscard meta::Handle<T> emplace_empty() noexcept {
        std::lock_guard<std::mutex> lock(mutex<T>);
        assets<T>.emplace_back();
        return { assets<T>.size() - 1 };
    }

    template <typename T>
    qz_nodiscard T& from_handle(meta::Handle<T> handle) noexcept {
        return assets<T>[handle.index].first;
    }

    template <typename T>
    qz_nodiscard bool is_ready(meta::Handle<T> handle) noexcept {
        return assets<T>[handle.index].second;
    }

    template <typename T>
    void finalize(meta::Handle<T> handle, T&& data) noexcept {
        std::lock_guard<std::mutex> lock(mutex<T>);
        assets<T>[handle.index] = {
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
        return assets<gfx::StaticTexture>[0].first;
    }

    std::vector<VkDescriptorImageInfo> all_textures(const gfx::Context& context) noexcept {
        std::lock_guard<std::mutex> lock(mutex<gfx::StaticTexture>);
        std::vector<VkDescriptorImageInfo> descriptors{};
        descriptors.reserve(assets<gfx::StaticTexture>.size());
        for (const auto& [texture, ready] : assets<gfx::StaticTexture>) {
            if (ready) [[likely]] {
                descriptors.push_back({
                    .sampler = context.default_sampler,
                    .imageView = texture.view(),
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                });
            } else [[unlikely]] {
                descriptors.push_back({
                    .sampler = context.default_sampler,
                    .imageView = default_texture().view(),
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                });
            }
        }
        return descriptors;
    }

    template <typename T>
    void lock() noexcept {
        mutex<T>.lock();
    }

    template <typename T>
    void unlock() noexcept {
        mutex<T>.unlock();
    }
    
    template meta::Handle<gfx::StaticMesh>    emplace_empty() noexcept;
    template meta::Handle<gfx::StaticTexture> emplace_empty() noexcept;

    template gfx::StaticMesh&    from_handle(meta::Handle<gfx::StaticMesh>)    noexcept;
    template gfx::StaticTexture& from_handle(meta::Handle<gfx::StaticTexture>) noexcept;

    template bool is_ready(meta::Handle<gfx::StaticMesh>)    noexcept;
    template bool is_ready(meta::Handle<gfx::StaticTexture>) noexcept;

    template void finalize(meta::Handle<gfx::StaticMesh>, gfx::StaticMesh&&)       noexcept;
    template void finalize(meta::Handle<gfx::StaticTexture>, gfx::StaticTexture&&) noexcept;

    template void lock<gfx::StaticMesh>()    noexcept;
    template void lock<gfx::StaticTexture>() noexcept;

    template void unlock<gfx::StaticMesh>()    noexcept;
    template void unlock<gfx::StaticTexture>() noexcept;
} // namespace qz::assets