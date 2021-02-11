#include <qz/gfx/descriptor_set.hpp>
#include <qz/gfx/static_texture.hpp>
#include <qz/gfx/static_model.hpp>
#include <qz/gfx/render_pass.hpp>
#include <qz/gfx/static_mesh.hpp>
#include <qz/gfx/pipeline.hpp>
#include <qz/gfx/renderer.hpp>
#include <qz/gfx/context.hpp>
#include <qz/gfx/buffer.hpp>
#include <qz/gfx/window.hpp>
#include <qz/gfx/assets.hpp>
#include <qz/gfx/queue.hpp>

#include <qz/meta/constants.hpp>

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace qz;

struct Camera {
    struct Raw {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view;
    };
    glm::vec3 position = {1.2f, 0.4f, 0.0f };
    glm::vec3 front = { 0.0f, 0.0f, -1.0f };
    glm::vec3 up = { 0.0f, 1.0f, 0.0f };
    glm::vec3 right = { 0.0f, 0.0f, 0.0f };
    glm::vec3 world_up = { 0.0f, 1.0f, 0.0f };
    float yaw = -180.0f;
    float pitch = 0.0f;

    void update(const gfx::Window& window, double delta_time) noexcept {
        _process_keyboard(window, delta_time);
        _process_mouse(window);

        const auto cos_pitch = std::cos(glm::radians(pitch));
        front = glm::normalize(glm::vec3{
            std::cos(glm::radians(yaw)) * cos_pitch,
            std::sin(glm::radians(pitch)),
            std::sin(glm::radians(yaw)) * cos_pitch
        });
        right = glm::normalize(glm::cross(front, world_up));
        up = glm::normalize(glm::cross(right, front));
    }

    qz_nodiscard glm::mat4 view() const noexcept {
        return glm::lookAt(position, position + front, up);
    }
private:
    void _process_keyboard(const gfx::Window& window, double delta_time) noexcept {
        constexpr auto camera_speed = 1.5f;
        const auto delta_movement = camera_speed * (float)delta_time;
        if (window.get_key_state(gfx::Keys::W) == gfx::KeyState::pressed) {
            position.x += std::cos(glm::radians(yaw)) * delta_movement;
            position.z += std::sin(glm::radians(yaw)) * delta_movement;
        }
        if (window.get_key_state(gfx::Keys::S) == gfx::KeyState::pressed) {
            position.x -= std::cos(glm::radians(yaw)) * delta_movement;
            position.z -= std::sin(glm::radians(yaw)) * delta_movement;
        }
        if (window.get_key_state(gfx::Keys::A) == gfx::KeyState::pressed) {
            position -= right * delta_movement;
        }
        if (window.get_key_state(gfx::Keys::D) == gfx::KeyState::pressed) {
            position += right * delta_movement;
        }
        if (window.get_key_state(gfx::Keys::space) == gfx::KeyState::pressed) {
            position += world_up * delta_movement;
        }
        if (window.get_key_state(gfx::Keys::left_shift) == gfx::KeyState::pressed) {
            position -= world_up * delta_movement;
        }
    }

    void _process_mouse(const gfx::Window& window) noexcept {
        const auto [xoff, yoff] = window.get_mouse_offset();
        yaw += (float)xoff;
        pitch += (float)yoff;

        if (pitch > 89.9f) {
            pitch = 89.9f;
        }
        if (pitch < -89.9f) {
            pitch = -89.9f;
        }
    }
};

int main() {
    auto window = gfx::Window::create(1280, 720, "QuartzVk");
    auto context = gfx::Context::create();
    auto renderer = gfx::Renderer::create(context, window);

    auto render_pass = gfx::RenderPass::create(context, {
        .attachments = { {
            .image = gfx::Image::create(context, {
                .width = 1280,
                .height = 720,
                .mips = 1,
                .format = renderer.swapchain.format,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            }),
            .name = "color",
            .framebuffers = { 0 },
            .owning = true,
            .discard = false,
            .layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .clear = gfx::ClearColor{}
        }, {
            .image = gfx::Image::create(context, {
                .width = 1280,
                .height = 720,
                .mips = 1,
                .format = VK_FORMAT_D32_SFLOAT_S8_UINT,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            }),
            .name = "depth",
            .framebuffers = { 0 },
            .owning = true,
            .discard = true,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .clear = gfx::ClearDepth{ 1.0f, 0 }
        } },
        .subpasses = { {
            .attachments = {
                "color",
                "depth"
            }
        } },
        .dependencies = { {
            .source_subpass = meta::external_subpass,
            .dest_subpass = 0,
            .source_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dest_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .source_access = {},
            .dest_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        } }
    });

    auto pipeline = gfx::Pipeline::create(context, renderer, {
        .vertex = "../data/shaders/shader.vert.spv",
        .fragment = "../data/shaders/shader.frag.spv",
        .attributes = {
            gfx::VertexAttribute::vec3,
            gfx::VertexAttribute::vec3,
            gfx::VertexAttribute::vec2,
            gfx::VertexAttribute::vec3,
            gfx::VertexAttribute::vec3
        },
        .states = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .render_pass = &render_pass,
        .subpass = 0,
        .depth = true
    });

    auto scene = gfx::StaticModel::request(context, "../data/models/sponza/sponza.glb");
    auto set = gfx::DescriptorSet<>::allocate(context, pipeline.set(0));
    auto buffer = gfx::Buffer<>::allocate(context, sizeof(Camera::Raw), meta::uniform_buffer);

    Camera camera;
    Camera::Raw camera_data = {
        glm::perspective(glm::radians(60.0f), window.width() / (float)window.height(), 0.1f, 100.0f),
        glm::scale(glm::mat4(1.0f), glm::vec3(0.005f)),
        camera.view()
    };

    std::size_t frame_count = 0;
    double delta_time = 0, last_frame = 0;
    while (!window.should_close()) {
        auto [command_buffer, frame] = gfx::acquire_next_frame(renderer, context);
        ++frame_count;

        const auto current_frame = gfx::get_time();
        delta_time = current_frame - last_frame;
        last_frame = current_frame;

        camera_data.view = camera.view();
        buffer[frame.index].write(&camera_data, meta::whole_size);
        gfx::DescriptorSet<1>::bind(context, set[frame.index], pipeline["Camera"], buffer[frame.index]);
        gfx::DescriptorSet<1>::bind(context, set[frame.index], pipeline["textures"], assets::all_textures(context));

        command_buffer
            .begin()
                .begin_render_pass(render_pass, 0)
                .set_viewport(meta::full_viewport)
                .set_scissor(meta::full_scissor)
                .bind_pipeline(pipeline)
                .bind_descriptor_set(set[frame.index]);

        if (const auto lock = assets::acquire<gfx::StaticModel>(); assets::is_ready(scene)) qz_likely {
            for (const auto& [mesh, diffuse, normal, specular, vertex, index] : assets::from_handle(scene).submeshes) {
                command_buffer
                    .bind_static_mesh(mesh)
                    .push_constants(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(std::uint32_t), &diffuse.index)
                    .draw_indexed(index, 1, 0, 0);
            }
        }

        command_buffer
                .end_render_pass()
                .insert_layout_transition({
                    .image = frame.image,
                    .source_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                    .source_access = {},
                    .dest_access = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .old_layout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                })
                .copy_image(render_pass.image("color"), *frame.image)
                .insert_layout_transition({
                    .image = frame.image,
                    .source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                    .dest_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    .source_access = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .dest_access = {},
                    .old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .new_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                })
            .end();

        gfx::present_frame(renderer, context, command_buffer, frame, render_pass.sync_stage());
        gfx::poll_transfers(context);
        window.poll_events();
        camera.update(window, delta_time);
    }

    context.graphics->wait_idle();
    assets::free_all_resources(context);

    gfx::DescriptorSet<>::destroy(context, set);
    gfx::Buffer<>::destroy(context, buffer);
    gfx::Pipeline::destroy(context, pipeline);
    gfx::RenderPass::destroy(context, render_pass);

    gfx::Renderer::destroy(context, renderer);
    gfx::Context::destroy(context);
    gfx::Window::destroy(window);
    gfx::Window::terminate();
    return 0;
}
