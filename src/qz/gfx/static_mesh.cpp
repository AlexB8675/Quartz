#include <qz/gfx/command_buffer.hpp>
#include <qz/gfx/static_mesh.hpp>
#include <qz/gfx/context.hpp>
#include <qz/gfx/assets.hpp>
#include <qz/gfx/queue.hpp>

#include <qz/meta/types.hpp>

#include <numeric>
#include <cstring>

namespace qz::gfx {
    struct TaskData {
        const Context* context;
        meta::Handle<StaticMesh> handle;
        std::vector<float> vertices;
        std::vector<std::uint32_t> indices;
    };

    qz_nodiscard meta::Handle<StaticMesh> StaticMesh::request(const Context& context, StaticMesh::CreateInfo&& info) noexcept {
        const auto result = assets::emplace_empty<StaticMesh>();
        const auto task_data = new TaskData{
            &context,
            result,
            std::move(info.geometry),
            std::move(info.indices),
        };

        context.scheduler->AddTask(ftl::Task{
            .Function = +[](ftl::TaskScheduler* scheduler, void* ptr) {
                const auto data = static_cast<const TaskData*>(ptr);
                const auto thread_index = scheduler->GetCurrentThreadIndex();
                const auto& context = *data->context;
                auto command_buffer = CommandBuffer::allocate(context, context.transfer_pools[thread_index]);

                auto vertex_staging = StaticBuffer::create(context, {
                    .flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    .usage = VMA_MEMORY_USAGE_CPU_ONLY,
                    .capacity = data->vertices.size() * sizeof(float)
                });
                std::memcpy(vertex_staging.mapped, data->vertices.data(), vertex_staging.capacity);

                auto index_staging = StaticBuffer::create(context, {
                    .flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    .usage = VMA_MEMORY_USAGE_CPU_ONLY,
                    .capacity = data->indices.size() * sizeof(std::uint32_t)
                });
                std::memcpy(index_staging.mapped, data->indices.data(), index_staging.capacity);

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

                command_buffer
                    .begin()
                        .copy_buffer(vertex_staging, geometry)
                        .copy_buffer(index_staging, indices)
                        .transfer_ownership({
                            .buffer = &geometry,
                            .source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                            .dest_stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                            .source_access = VK_ACCESS_TRANSFER_WRITE_BIT,
                            .dest_access = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
                        }, *context.transfer, *context.graphics)
                        .transfer_ownership({
                            .buffer = &indices,
                            .source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                            .dest_stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                            .source_access = VK_ACCESS_TRANSFER_WRITE_BIT,
                            .dest_access = VK_ACCESS_INDEX_READ_BIT
                        }, *context.transfer, *context.graphics)
                    .end();

                VkFenceCreateInfo fence_create_info{};
                fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                VkFence done{};
                qz_vulkan_check(vkCreateFence(context.device, &fence_create_info, nullptr, &done));
                context.transfer->submit(command_buffer, {}, nullptr, nullptr, done);
                while (vkGetFenceStatus(context.device, done) != VK_SUCCESS);
                assets::finalize(data->handle, {
                    geometry,
                    indices
                });
                vkDestroyFence(context.device, done, nullptr);
                StaticBuffer::destroy(context, vertex_staging);
                StaticBuffer::destroy(context, index_staging);
                CommandBuffer::destroy(context, command_buffer);
                delete data;
            },
            .ArgData = task_data
        }, ftl::TaskPriority::High);
        return result;
    }
} // namespace qz::gfx