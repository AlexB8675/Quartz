#pragma once


#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <ftl/task_scheduler.h>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace qz::gfx {
    struct Settings {
        std::uint32_t version = VK_MAKE_VERSION(1, 2, 0);
        // TODO: Maybe more settings?
    };

    struct Context {
        VkInstance instance;
#if defined(quartz_debug)
        VkDebugUtilsMessengerEXT validation;
#endif
        VkPhysicalDevice gpu;
        VkDevice device;
        VmaAllocator allocator;
        std::unique_ptr<ftl::TaskScheduler> scheduler;
        std::unique_ptr<Queue> graphics;
        std::unique_ptr<Queue> transfer;
        VkCommandPool main_pool;
        std::vector<VkCommandPool> transfer_pools;
        VkDescriptorPool descriptor_pool;

        qz_nodiscard static Context create(const Settings& = {}) noexcept;
        static void destroy(Context&) noexcept;
    };
} // namespace qz::gfx