#include <qz/gfx/task_manager.hpp>

namespace qz::gfx {
    ftl::TaskScheduler& TaskManager::handle() noexcept {
        return _handle;
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

        for (auto it = _tasks.begin(); it != _tasks.end(); ++it) {
            if (it->poll() == VK_SUCCESS) {
                it->cleanup();
                if ((it = _tasks.erase(it)) == _tasks.end()) {
                    break;
                }
            }
        }
    }
} // namespace qz::gfx