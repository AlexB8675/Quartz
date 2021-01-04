#include <qz/gfx/command_buffer.hpp>
#include <qz/gfx/swapchain.hpp>
#include <qz/gfx/context.hpp>
#include <qz/gfx/queue.hpp>

namespace qz::gfx {
    qz_nodiscard std::unique_ptr<Queue> Queue::create(const Context& context,
                                                       std::uint32_t family,
                                                       std::uint32_t index) noexcept {
        auto queue = std::make_unique<Queue>();
        queue->_family = family;
        vkGetDeviceQueue(context.device, family, index, &queue->_handle);
        return queue;
    }

    void Queue::submit(const CommandBuffer& commands,
                       VkPipelineStageFlags stage,
                       VkSemaphore wait,
                       VkSemaphore signal,
                       VkFence fence) noexcept {
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        if (wait) {
            submit_info.pWaitDstStageMask = &stage;
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &wait;
        }
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = commands.ptr_handle();
        if (signal) {
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &signal;
        }

        std::lock_guard<std::mutex> lock(_mutex);
        qz_vulkan_check(vkQueueSubmit(_handle, 1, &submit_info, fence));
    }

    void Queue::present(const Swapchain& swapchain, std::uint32_t image, VkSemaphore wait) noexcept {
        VkResult present_result{};
        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        if (wait) {
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &wait;
        }
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain.handle;
        present_info.pImageIndices = &image;
        present_info.pResults = &present_result;

        std::lock_guard<std::mutex> lock(_mutex);
        qz_vulkan_check(vkQueuePresentKHR(_handle, &present_info));
        qz_vulkan_check(present_result);
    }

    void Queue::wait_idle() const noexcept {
        qz_vulkan_check(vkQueueWaitIdle(_handle));
    }

    qz_nodiscard std::uint32_t Queue::family() const noexcept {
        return _family;
    }
} // namespace qz::gfx