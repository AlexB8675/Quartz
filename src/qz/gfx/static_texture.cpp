#include <qz/gfx/static_texture.hpp>
#include <qz/gfx/command_buffer.hpp>
#include <qz/gfx/static_buffer.hpp>
#include <qz/gfx/task_manager.hpp>
#include <qz/gfx/context.hpp>
#include <qz/gfx/assets.hpp>
#include <qz/gfx/queue.hpp>

#include <qz/util/file_view.hpp>

#include <stb_image.h>

#include <optional>
#include <cstring>
#include <thread>
#include <cmath>

namespace qz::gfx {
    template <>
    struct TaskData<StaticTexture> {
        VkFormat format;
        std::string path;
        const Context* context;
        meta::Handle<StaticTexture> result;
    };

    static void load_texture(ftl::TaskScheduler* scheduler, void* ptr) {
        const auto* task_data = static_cast<const TaskData<StaticTexture>*>(ptr);
        const auto thread_index = scheduler->GetCurrentThreadIndex();
        const auto& context = *task_data->context;
        auto file = util::FileView::create(task_data->path);

        std::int32_t width, height, channels = 4;
        const auto* image_data = stbi_load_from_memory((const std::uint8_t*)file.data(), file.size(), &width, &height, &channels, STBI_rgb_alpha);
        util::FileView::destroy(file);

        auto image = Image::create(context, {
            .width = (std::uint32_t)width,
            .height = (std::uint32_t)height,
            .mips = (std::uint32_t)std::floor(std::log2(std::max(width, height))) + 1,
            .format = task_data->format,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                     VK_IMAGE_USAGE_SAMPLED_BIT
        });

        auto staging = StaticBuffer::create(context, {
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY,
            (std::size_t)width * height * 4
        });
        std::memcpy(staging.mapped, image_data, staging.capacity);

        auto transfer_cmd = CommandBuffer::allocate(context, context.transfer_pools[thread_index]);
        transfer_cmd
            .begin()
                .insert_layout_transition({
                    .image = &image,
                    .mip = 0,
                    .levels = 0,
                    .source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    .dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                    .source_access = {},
                    .dest_access = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .old_layout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                })
                .copy_buffer_to_image(staging, image)
                .transfer_ownership({
                    .image = &image,
                    .mip = 0,
                    .levels = 0,
                    .source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                    .dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                    .source_access = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .dest_access = {},
                    .old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .new_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                }, *context.transfer, *context.graphics)
            .end();

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkSemaphore transfer_done, graphics_done;
        qz_vulkan_check(vkCreateSemaphore(context.device, &semaphore_info, nullptr, &transfer_done));
        qz_vulkan_check(vkCreateSemaphore(context.device, &semaphore_info, nullptr, &graphics_done));
        context.transfer->submit(transfer_cmd, {}, nullptr, transfer_done, nullptr);

        auto ownership_cmd = CommandBuffer::allocate(context, context.transient_pools[thread_index]);
        ownership_cmd
            .begin()
                .transfer_ownership({
                    .image = &image,
                    .mip = 0,
                    .levels = 0,
                    .source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                    .dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                    .source_access = {},
                    .dest_access = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
                    .old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .new_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                }, *context.transfer, *context.graphics)
            .end();
        context.graphics->submit(ownership_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, transfer_done, graphics_done, nullptr);

        VkFenceCreateInfo fence_create_info{};
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence request_done;
        qz_vulkan_check(vkCreateFence(context.device, &fence_create_info, nullptr, &request_done));
        auto mipmaps_cmd = CommandBuffer::allocate(context, context.transient_pools[thread_index]);
        mipmaps_cmd.begin();
        for (std::uint32_t mip = 1; mip < image.mips; ++mip) {
            mipmaps_cmd
                .insert_layout_transition({
                    .image = &image,
                    .mip = mip,
                    .levels = 1,
                    .source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                    .dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                    .source_access = {},
                    .dest_access = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .old_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    .new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                })
                .blit_image({
                    .source_image = &image,
                    .dest_image = nullptr,
                    .source_off = {
                        (std::int32_t)image.width >> (mip - 1),
                        (std::int32_t)image.height >> (mip - 1),
                        1
                    },
                    .dest_off = {
                        (std::int32_t)image.width >> mip,
                        (std::int32_t)image.height >> mip,
                        1
                    },
                    .source_mip = mip - 1,
                    .dest_mip = mip
                });
            if (mip != image.mips - 1) {
                mipmaps_cmd.insert_layout_transition({
                    .image = &image,
                    .mip = mip,
                    .levels = 1,
                    .source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                    .dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                    .source_access = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .dest_access = VK_ACCESS_TRANSFER_READ_BIT,
                    .old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .new_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                });
            } else {
                mipmaps_cmd.insert_layout_transition({
                    .image = &image,
                    .mip = mip,
                    .levels = 1,
                    .source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                    .dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    .source_access = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .dest_access = VK_ACCESS_SHADER_READ_BIT,
                    .old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                });
            }
        }
        mipmaps_cmd
                .insert_layout_transition({
                    .image = &image,
                    .mip = 0,
                    .levels = std::max(image.mips - 1, 1u),
                    .source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                    .dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    .source_access = VK_ACCESS_TRANSFER_READ_BIT,
                    .dest_access = VK_ACCESS_SHADER_READ_BIT,
                    .old_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    .new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                })
            .end();
        context.graphics->submit(mipmaps_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, graphics_done, nullptr, request_done);
        assets::finalize(task_data->result, StaticTexture::from_raw(image));
        vkWaitForFences(context.device, 1, &request_done, true, -1);
        vkDestroySemaphore(context.device, graphics_done, nullptr);
        vkDestroySemaphore(context.device, transfer_done, nullptr);
        vkDestroyFence(context.device, request_done, nullptr);
        StaticBuffer::destroy(context, staging);
        CommandBuffer::destroy(context, ownership_cmd);
        CommandBuffer::destroy(context, transfer_cmd);
        delete task_data;
    }

    qz_nodiscard StaticTexture StaticTexture::from_raw(const Image& handle) noexcept {
        StaticTexture texture{};
        texture._handle = handle;
        return texture;
    }

    qz_nodiscard meta::Handle<StaticTexture> StaticTexture::allocate(const Context& context, std::string_view path, VkFormat format) noexcept {
        using namespace std::literals;
        const auto result = request(context, path, format);
        while (true) {
            {
                const auto lock = assets::acquire<StaticTexture>();
                qz_unlikely_if(assets::is_ready(result)) {
                    break;
                }
            }
            std::this_thread::sleep_for(1ms);
            poll_transfers(context);
        }
        return result;
    }

    meta::Handle<StaticTexture> StaticTexture::request(const Context& context, std::string_view path, VkFormat format) noexcept {
        const auto result = assets::emplace_empty<StaticTexture>();
        context.task_manager->add_task({
            .Function = load_texture,
            .ArgData = new TaskData<StaticTexture>{
                format,
                path.data(),
                &context,
                result,
            }
        });
        return result;
    }

    void StaticTexture::destroy(const Context& context, StaticTexture& texture) noexcept {
        Image::destroy(context, texture._handle);
        texture = {};
    }

    qz_nodiscard VkImageView StaticTexture::view() const noexcept {
        return _handle.view;
    }
} // namespace qz::gfx