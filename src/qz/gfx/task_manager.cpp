#include <qz/gfx/task_manager.hpp>

namespace qz::gfx {
    ftl::TaskScheduler& TaskManager::handle() noexcept {
        return _handle;
    }

    void TaskManager::add_task(ftl::Task&& task) noexcept {
        _handle.AddTask(task, ftl::TaskPriority::High);
    }

    void TaskManager::insert(TaskStub&& stub) noexcept {
        std::lock_guard<std::mutex> lock(_mutex);
        _tasks.emplace_back(stub);
    }

    void TaskManager::tick() noexcept {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_tasks.empty()) {
            return;
        }

        for (std::size_t i = 0; i < _tasks.size(); ++i) {
            if (_tasks[i].poll() == VK_SUCCESS) {
                _tasks[i].cleanup();
                _tasks.erase(_tasks.begin() + i++);
            }
        }
    }
} // namespace qz::gfx