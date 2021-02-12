#include <qz/gfx/command_buffer.hpp>
#include <qz/gfx/task_manager.hpp>
#include <qz/gfx/static_mesh.hpp>
#include <qz/gfx/context.hpp>
#include <qz/gfx/assets.hpp>
#include <qz/gfx/queue.hpp>

#include <qz/meta/types.hpp>

#include <numeric>
#include <cstring>

namespace qz::gfx {
    template <>
    struct TaskData<StaticMesh> {
        const Context* context;
        meta::Handle<StaticMesh> result;
        std::vector<float> vertices;
        std::vector<std::uint32_t> indices;
    };

    qz_nodiscard meta::Handle<StaticMesh> StaticMesh::request(const Context& context, StaticMesh::CreateInfo&& info) noexcept {
        const auto result = assets::emplace_empty<StaticMesh>();

        context.task_manager->add_task(ftl::Task{
            .Function = +[](ftl::TaskScheduler* scheduler, void* ptr) {
                const auto* task_data = static_cast<const TaskData<StaticMesh>*>(ptr);
                const auto thread_index = scheduler->GetCurrentThreadIndex();
                const auto& context = *task_data->context;

                auto vertex_staging = StaticBuffer::create(context, {
                    .flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    .usage = VMA_MEMORY_USAGE_CPU_ONLY,
                    .capacity = task_data->vertices.size() * sizeof(float)
                });
                std::memcpy(vertex_staging.mapped, task_data->vertices.data(), vertex_staging.capacity);

                auto index_staging = StaticBuffer::create(context, {
                    .flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    .usage = VMA_MEMORY_USAGE_CPU_ONLY,
                    .capacity = task_data->indices.size() * sizeof(std::uint32_t)
                });
                std::memcpy(index_staging.mapped, task_data->indices.data(), index_staging.capacity);

                auto geometry = StaticBuffer::create(context, {
                    .flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                    .capacity = vertex_staging.capacity
                });
                auto indices = StaticBuffer::create(context, {
                    .flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                    .capacity = index_staging.capacity
                });

                auto transfer_cmd = CommandBuffer::allocate(context, context.transfer_pools[thread_index]);
                transfer_cmd
                    .begin()
                        .copy_buffer(vertex_staging, geometry)
                        .copy_buffer(index_staging, indices)
                        .transfer_ownership({
                            .buffer = &geometry,
                            .source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                            .dest_stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                            .source_access = VK_ACCESS_TRANSFER_WRITE_BIT,
                            .dest_access = {}
                        }, *context.transfer, *context.graphics)
                        .transfer_ownership({
                            .buffer = &indices,
                            .source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                            .dest_stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                            .source_access = VK_ACCESS_TRANSFER_WRITE_BIT,
                            .dest_access = {}
                        }, *context.transfer, *context.graphics)
                    .end();

                VkSemaphoreCreateInfo transfer_semaphore_info{};
                transfer_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                VkSemaphore transfer_done;
                qz_vulkan_check(vkCreateSemaphore(context.device, &transfer_semaphore_info, nullptr, &transfer_done));
                context.transfer->submit(transfer_cmd, {}, nullptr, transfer_done, nullptr);

                auto ownership_cmd = CommandBuffer::allocate(context, context.transient_pools[thread_index]);
                ownership_cmd
                    .begin()
                        .transfer_ownership({
                            .buffer = &geometry,
                            .source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                            .dest_stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                            .source_access = {},
                            .dest_access = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
                        }, *context.transfer, *context.graphics)
                        .transfer_ownership({
                            .buffer = &indices,
                            .source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                            .dest_stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                            .source_access = {},
                            .dest_access = VK_ACCESS_INDEX_READ_BIT
                        }, *context.transfer, *context.graphics)
                    .end();

                VkFenceCreateInfo fence_create_info{};
                fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                VkFence request_done;
                qz_vulkan_check(vkCreateFence(context.device, &fence_create_info, nullptr, &request_done));
                context.graphics->submit(ownership_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, transfer_done, nullptr, request_done);
                while (vkGetFenceStatus(context.device, request_done) != VK_SUCCESS);
                vkDestroySemaphore(context.device, transfer_done, nullptr);
                vkDestroyFence(context.device, request_done, nullptr);
                StaticBuffer::destroy(context, vertex_staging);
                StaticBuffer::destroy(context, index_staging);
                CommandBuffer::destroy(context, ownership_cmd);
                CommandBuffer::destroy(context, transfer_cmd);
                assets::finalize(task_data->result, {
                    geometry,
                    indices
                });
                delete task_data;
            },
            .ArgData = new TaskData<StaticMesh>{
                &context,
                result,
                std::move(info.geometry),
                std::move(info.indices),
            }
        });
        return result;
    }

    void StaticMesh::destroy(const Context& context, StaticMesh& mesh) noexcept {
        gfx::StaticBuffer::destroy(context, mesh.geometry);
        gfx::StaticBuffer::destroy(context, mesh.indices);
        mesh = {};
    }
} // namespace qz::gfx