cmake_minimum_required(VERSION 3.17)
project(quartz CXX)

find_package(Vulkan REQUIRED)

# Disable GLFW docs, tests and examples builds and forces to use statically linked vulkan loader.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC ON CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(ext/glfw)

# Disable SPIRV-Cross tests and exceptions, force build as a static library.
set(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(ext/SPIRV-Cross)

# Disable FTL Tests, Benchmarks and enable C++17 Standard.
set(FTL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FTL_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(FTL_CPP_17 ON CACHE BOOL "" FORCE)
add_subdirectory(ext/FiberTaskingLib)

# Disable all annoying ASSIMP shit, tests, exporters and so on.
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_3DS_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(ext/assimp)

if (WIN32)
    target_compile_options(assimp PRIVATE /EHsc)
endif()

add_subdirectory(ext/glm)

set(CMAKE_CXX_STANDARD 20)

add_executable(quartz
    src/qz/gfx/assets.cpp
    src/qz/gfx/assets.hpp
    src/qz/gfx/buffer.cpp
    src/qz/gfx/buffer.hpp
    src/qz/gfx/clear.hpp
    src/qz/gfx/command_buffer.cpp
    src/qz/gfx/command_buffer.hpp
    src/qz/gfx/context.cpp
    src/qz/gfx/context.hpp
    src/qz/gfx/descriptor_set.cpp
    src/qz/gfx/descriptor_set.hpp
    src/qz/gfx/image.cpp
    src/qz/gfx/image.hpp
    src/qz/gfx/pipeline.cpp
    src/qz/gfx/pipeline.hpp
    src/qz/gfx/queue.cpp
    src/qz/gfx/queue.hpp
    src/qz/gfx/render_pass.cpp
    src/qz/gfx/render_pass.hpp
    src/qz/gfx/renderer.cpp
    src/qz/gfx/renderer.hpp
    src/qz/gfx/static_buffer.cpp
    src/qz/gfx/static_buffer.hpp
    src/qz/gfx/static_mesh.cpp
    src/qz/gfx/static_mesh.hpp
    src/qz/gfx/static_model.cpp
    src/qz/gfx/static_model.hpp
    src/qz/gfx/static_texture.cpp
    src/qz/gfx/static_texture.hpp
    src/qz/gfx/stb_image.cpp
    src/qz/gfx/task_manager.cpp
    src/qz/gfx/task_manager.hpp
    src/qz/gfx/swapchain.cpp
    src/qz/gfx/swapchain.hpp
    src/qz/gfx/vma.cpp
    src/qz/gfx/window.cpp
    src/qz/gfx/window.hpp

    src/qz/meta/constants.hpp
    src/qz/meta/types.hpp

    src/qz/util/file_view.cpp
    src/qz/util/file_view.hpp
    src/qz/util/fwd.hpp
    src/qz/util/hash.hpp
    src/qz/util/macros.hpp

    src/main.cpp)

target_compile_definitions(quartz PUBLIC

    # Set DEBUG macro if in Debug mode.
    $<$<CONFIG:Debug>:quartz_debug>

    # Removes annoying Window's shit.
    $<$<BOOL:${WIN32}>:
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX>

    # Misc
    VK_ENABLE_BETA_EXTENSIONS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RADIANS)

target_include_directories(quartz PUBLIC
    src
    ext/glm
    ext/stb
    ext/VulkanMemoryAllocator/src)

target_link_libraries(quartz PUBLIC
    ftl
    glfw
    assimp
    Vulkan::Vulkan
    spirv-cross-glsl)
