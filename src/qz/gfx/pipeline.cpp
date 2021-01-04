#include <qz/gfx/render_pass.hpp>
#include <qz/gfx/renderer.hpp>
#include <qz/gfx/pipeline.hpp>
#include <qz/gfx/context.hpp>

#include <spirv.hpp>
#include <spirv_glsl.hpp>

#include <type_traits>
#include <numeric>
#include <fstream>
#include <cstring>
#include <string>
#include <map>

namespace qz::gfx {
    qz_nodiscard static std::vector<char> load_spirv_code(const char* path) noexcept {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        qz_assert(file.is_open(), "failed to open file");

        std::vector<char> spirv(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(spirv.data(), spirv.size());
        return spirv;
    }

    qz_nodiscard Pipeline Pipeline::create(const Context& context, Renderer& renderer, CreateInfo&& info) noexcept {
        VkPipelineShaderStageCreateInfo pipeline_stages[2] = {};
        pipeline_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipeline_stages[0].pName = "main";

        pipeline_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipeline_stages[1].pName = "main";

        VkPushConstantRange push_constant_range{};
        push_constant_range.offset = 0;

        DescriptorBindings descriptor_types;
        std::vector<std::uint32_t> vertex_input_locations;
        std::map<std::size_t, DescriptorLayout> descriptor_layout;
        { // Vertex shader.
            const auto binary = load_spirv_code(info.vertex);
            const auto compiler = spirv_cross::CompilerGLSL((const std::uint32_t*)binary.data(), binary.size() / sizeof(std::uint32_t));
            const auto resources = compiler.get_shader_resources();

            VkShaderModuleCreateInfo module_create_info{};
            module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            module_create_info.codeSize = binary.size();
            module_create_info.pCode = (const uint32_t*)binary.data();
            qz_vulkan_check(vkCreateShaderModule(context.device, &module_create_info, nullptr, &pipeline_stages[0].module));

            for (const auto& uniform_buffer : resources.uniform_buffers) {
                const auto set_idx = compiler.get_decoration(uniform_buffer.id, spv::DecorationDescriptorSet);
                const auto binding_idx = compiler.get_decoration(uniform_buffer.id, spv::DecorationBinding);

                descriptor_layout[set_idx].push_back(
                    descriptor_types[uniform_buffer.name] = {
                        .dynamic = false,
                        .name    = uniform_buffer.name,
                        .index   = binding_idx,
                        .count   = 1,
                        .type    = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .stage   = VK_SHADER_STAGE_VERTEX_BIT
                    });
            }

            for (const auto& push_constant : resources.push_constant_buffers) {
                const auto& type = compiler.get_type(push_constant.type_id);
                push_constant_range.size = compiler.get_declared_struct_size(type);
                push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            }

            vertex_input_locations.reserve(resources.stage_inputs.size());
            for (const auto& vertex_input : resources.stage_inputs) {
                vertex_input_locations.emplace_back(compiler.get_decoration(vertex_input.id, spv::DecorationLocation));
            }
        }

        std::vector<VkPipelineColorBlendAttachmentState> attachment_outputs;
        { // Fragment shader.
            const auto binary = load_spirv_code(info.fragment);
            const auto compiler = spirv_cross::CompilerGLSL((const std::uint32_t*)binary.data(), binary.size() / sizeof(std::uint32_t));
            const auto resources = compiler.get_shader_resources();

            VkShaderModuleCreateInfo module_create_info{};
            module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            module_create_info.codeSize = binary.size();
            module_create_info.pCode = (const uint32_t*)binary.data();
            qz_vulkan_check(vkCreateShaderModule(context.device, &module_create_info, nullptr, &pipeline_stages[1].module));

            attachment_outputs.reserve(resources.stage_outputs.size());
            for (const auto& output : resources.stage_outputs) {
                attachment_outputs.push_back({
                    .blendEnable = true,
                    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    .colorBlendOp = VK_BLEND_OP_ADD,
                    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                    .alphaBlendOp = VK_BLEND_OP_ADD,
                    .colorWriteMask =
                        VK_COLOR_COMPONENT_R_BIT |
                        VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT |
                        VK_COLOR_COMPONENT_A_BIT
                });
            }

            for (const auto& uniform_buffer : resources.uniform_buffers) {
                const auto set_idx = compiler.get_decoration(uniform_buffer.id, spv::DecorationDescriptorSet);
                const auto binding_idx = compiler.get_decoration(uniform_buffer.id, spv::DecorationBinding);
                auto& layout = descriptor_layout[set_idx];

                if (auto it = layout.end(); (it = std::find_if(layout.begin(), layout.end(),
                        [binding_idx](const auto& each) {
                            return each.index == binding_idx;
                        })) != layout.end()) {
                    descriptor_types[uniform_buffer.name].stage =
                        (it->stage |= VK_SHADER_STAGE_FRAGMENT_BIT);
                } else {
                    layout.push_back(
                        descriptor_types[uniform_buffer.name] = {
                            .dynamic = false,
                            .name    = uniform_buffer.name,
                            .index   = binding_idx,
                            .count   = 1,
                            .type    = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                            .stage   = VK_SHADER_STAGE_FRAGMENT_BIT
                        });
                }
            }

            for (const auto& image : resources.sampled_images) {
                const auto set_idx = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
                const auto binding_idx = compiler.get_decoration(image.id, spv::DecorationBinding);
                const auto& image_type = compiler.get_type(image.type_id);
                const bool is_array = !image_type.array.empty();
                const bool is_dynamic = is_array && image_type.array[0] == 0;

                descriptor_layout[set_idx].push_back(
                    descriptor_types[image.name] = {
                        .dynamic = is_dynamic,
                        .name    = image.name,
                        .index   = binding_idx,
                        .count   = !is_array ? 1 : is_dynamic ? 4096 : image_type.array[0],
                        .type    = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .stage   = VK_SHADER_STAGE_FRAGMENT_BIT
                    });
            }

            for (const auto& push_constant : resources.push_constant_buffers) {
                const auto& type = compiler.get_type(push_constant.type_id);
                push_constant_range.size = compiler.get_declared_struct_size(type);
                push_constant_range.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
            }
        }

        VkVertexInputBindingDescription vertex_binding_description{};
        vertex_binding_description.binding = 0;
        vertex_binding_description.stride =
            std::accumulate(info.attributes.begin(), info.attributes.end(), 0u, [](const auto value, const auto attribute) noexcept {
                return value + static_cast<std::underlying_type_t<decltype(attribute)>>(attribute);
            });
        vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> vertex_attribute_descriptions;
        vertex_attribute_descriptions.reserve(info.attributes.size());
        for (std::uint32_t location = 0, offset = 0; const auto attribute : info.attributes) {
            vertex_attribute_descriptions.push_back({
                .location = vertex_input_locations[location++],
                .binding = 0,
                .format = [attribute]() noexcept {
                    switch (attribute) {
                        case VertexAttribute::vec1: return VK_FORMAT_R32_SFLOAT;
                        case VertexAttribute::vec2: return VK_FORMAT_R32G32_SFLOAT;
                        case VertexAttribute::vec3: return VK_FORMAT_R32G32B32_SFLOAT;
                        case VertexAttribute::vec4: return VK_FORMAT_R32G32B32A32_SFLOAT;
                    }
                    qz_unreachable();
                }(),
                .offset = offset
            });
            offset += static_cast<std::underlying_type_t<decltype(attribute)>>(attribute);
        }

        VkPipelineVertexInputStateCreateInfo vertex_input_state{};
        vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state.vertexBindingDescriptionCount = 1;
        vertex_input_state.pVertexBindingDescriptions = &vertex_binding_description;
        vertex_input_state.vertexAttributeDescriptionCount = vertex_attribute_descriptions.size();
        vertex_input_state.pVertexAttributeDescriptions = vertex_attribute_descriptions.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state{};
        input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly_state.primitiveRestartEnable = false;

        VkViewport viewport{};
        VkRect2D scissor{};

        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.pViewports = &viewport;
        viewport_state.scissorCount = 1;
        viewport_state.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer_state{};
        rasterizer_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer_state.depthClampEnable = false;
        rasterizer_state.rasterizerDiscardEnable = false;
        rasterizer_state.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer_state.lineWidth = 1.0f;
        rasterizer_state.cullMode = VK_CULL_MODE_NONE;
        rasterizer_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer_state.depthBiasEnable = false;
        rasterizer_state.depthBiasConstantFactor = 0.0f;
        rasterizer_state.depthBiasClamp = 0.0f;
        rasterizer_state.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling_state{};
        multisampling_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling_state.sampleShadingEnable = false;
        multisampling_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling_state.minSampleShading = 0.2f;
        multisampling_state.pSampleMask = nullptr;
        multisampling_state.alphaToCoverageEnable = false;
        multisampling_state.alphaToOneEnable = false;

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.depthTestEnable = info.depth;
        depth_stencil_state.depthWriteEnable = info.depth;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depth_stencil_state.depthBoundsTestEnable = false;
        depth_stencil_state.stencilTestEnable = false;
        depth_stencil_state.front = {};
        depth_stencil_state.back = {};
        depth_stencil_state.minDepthBounds = 0.0f;
        depth_stencil_state.maxDepthBounds = 1.0f;

        VkPipelineColorBlendStateCreateInfo color_blend_state{};
        color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state.logicOpEnable = false;
        color_blend_state.logicOp = VK_LOGIC_OP_NO_OP;
        color_blend_state.attachmentCount = attachment_outputs.size();
        color_blend_state.pAttachments = attachment_outputs.data();
        color_blend_state.blendConstants[0] = 0.0f;
        color_blend_state.blendConstants[1] = 0.0f;
        color_blend_state.blendConstants[2] = 0.0f;
        color_blend_state.blendConstants[3] = 0.0f;

        VkPipelineDynamicStateCreateInfo pipeline_dynamic_states{};
        pipeline_dynamic_states.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        pipeline_dynamic_states.dynamicStateCount = info.states.size();
        pipeline_dynamic_states.pDynamicStates = info.states.data();

        DescriptorSetLayouts set_layouts{};
        set_layouts.reserve(descriptor_layout.size());
        for (const auto& [_, descriptors] : descriptor_layout) {
            if (!renderer.layout_cache.contains(descriptors)) {
                std::vector<VkDescriptorBindingFlags> flags;
                flags.reserve(descriptors.size());
                std::vector<VkDescriptorSetLayoutBinding> bindings;
                bindings.reserve(descriptors.size());
                for (const auto& binding : descriptors) {
                    flags.emplace_back();
                    if (binding.dynamic) {
                        flags.back() =
                            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                            VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
                    }

                    bindings.push_back({
                        .binding = (std::uint32_t)binding.index,
                        .descriptorType = binding.type,
                        .descriptorCount = binding.count,
                        .stageFlags = binding.stage,
                        .pImmutableSamplers = nullptr
                    });
                }

                VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags{};
                binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
                binding_flags.bindingCount = flags.size();
                binding_flags.pBindingFlags = flags.data();

                VkDescriptorSetLayoutCreateInfo create_info{};
                create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                create_info.pNext = &binding_flags;
                create_info.flags = {};
                create_info.bindingCount = bindings.size();
                create_info.pBindings = bindings.data();
                qz_vulkan_check(vkCreateDescriptorSetLayout(context.device, &create_info, nullptr, &renderer.layout_cache[descriptors]));
            }
            set_layouts.push_back({ renderer.layout_cache[descriptors], descriptors });
        }

        std::vector<VkDescriptorSetLayout> set_layout_handles;
        set_layout_handles.reserve(set_layouts.size());
        for (const auto& layout : set_layouts) {
            set_layout_handles.emplace_back(layout.handle);
        }

        VkPipelineLayoutCreateInfo layout_create_info{};
        layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_create_info.setLayoutCount = set_layout_handles.size();
        layout_create_info.pSetLayouts = set_layout_handles.data();
        if (push_constant_range.size == 0) {
            layout_create_info.pushConstantRangeCount = 0;
            layout_create_info.pPushConstantRanges = nullptr;
        } else {
            layout_create_info.pushConstantRangeCount = 1;
            layout_create_info.pPushConstantRanges = &push_constant_range;
        }

        VkPipelineLayout layout;
        qz_vulkan_check(vkCreatePipelineLayout(context.device, &layout_create_info, nullptr, &layout));

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.stageCount = 2;
        pipeline_create_info.pStages = pipeline_stages;
        pipeline_create_info.pVertexInputState = &vertex_input_state;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state;
        pipeline_create_info.pViewportState = &viewport_state;
        pipeline_create_info.pRasterizationState = &rasterizer_state;
        pipeline_create_info.pMultisampleState = &multisampling_state;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pColorBlendState = &color_blend_state;
        pipeline_create_info.pDynamicState = &pipeline_dynamic_states;
        pipeline_create_info.layout = layout;
        pipeline_create_info.renderPass = info.render_pass->handle();
        pipeline_create_info.subpass = info.subpass;
        pipeline_create_info.basePipelineHandle = nullptr;
        pipeline_create_info.basePipelineIndex = -1;

        VkPipeline handle;
        qz_vulkan_check(vkCreateGraphicsPipelines(context.device, nullptr, 1, &pipeline_create_info, nullptr, &handle));
        vkDestroyShaderModule(context.device, pipeline_stages[0].module, nullptr);
        vkDestroyShaderModule(context.device, pipeline_stages[1].module, nullptr);

        Pipeline pipeline{};
        pipeline._handle = handle;
        pipeline._bindings = std::move(descriptor_types);
        pipeline._layout = layout;
        pipeline._descriptors = std::move(set_layouts);
        return pipeline;
    }

    void Pipeline::destroy(const Context& context, Pipeline& pipeline) noexcept {
        vkDestroyPipeline(context.device, pipeline._handle, nullptr);
        vkDestroyPipelineLayout(context.device, pipeline._layout, nullptr);
        pipeline = {};
    }

    qz_nodiscard VkPipeline Pipeline::handle() const noexcept {
        return _handle;
    }

    qz_nodiscard VkPipelineLayout Pipeline::layout() const noexcept {
        return _layout;
    }

    const DescriptorSetLayout& Pipeline::set(std::size_t index) const noexcept {
        return _descriptors.at(index);
    }

    const DescriptorBinding& Pipeline::operator [](std::string_view name) const noexcept {
        return _bindings.at(name.data());
    }
} // namespace qz::gfx
