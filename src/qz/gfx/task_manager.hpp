#pragma once

#include <qz/util/macros.hpp>
#include <qz/util/fwd.hpp>

#include <vulkan/vulkan.h>

#include <ftl/task_scheduler.h>

#include <functional>
#include <vector>
#include <mutex>

namespace qz::gfx {
    struct TaskStub {
        std::function<VkResult()> poll;
        std::function<void()> cleanup;
    };

    class TaskManager {
        std::vector<TaskStub> _tasks;
        ftl::TaskScheduler _handle;
        std::mutex _mutex;
    public:
        qz_nodiscard ftl::TaskScheduler& handle() noexcept;
        void insert(TaskStub&&) noexcept;
        void tick() noexcept;
    };
} // namespace qz::gfx