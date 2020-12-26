#pragma once

#include <cassert>
#include <cstdlib>
#include <cstdio>

#define qz_for_each_0(f)
#define qz_for_each_1(f, x, ...) f(x)
#define qz_for_each_2(f, x, ...) f(x), qz_for_each_1(f, __VA_ARGS__)
#define qz_for_each_3(f, x, ...) f(x), qz_for_each_2(f, __VA_ARGS__)
#define qz_for_each_4(f, x, ...) f(x), qz_for_each_3(f, __VA_ARGS__)
#define qz_for_each_5(f, x, ...) f(x), qz_for_each_4(f, __VA_ARGS__)
#define qz_for_each_6(f, x, ...) f(x), qz_for_each_5(f, __VA_ARGS__)
#define qz_for_each_7(f, x, ...) f(x), qz_for_each_6(f, __VA_ARGS__)
#define qz_for_each_8(f, x, ...) f(x), qz_for_each_7(f, __VA_ARGS__)

#define qz_concat_impl(x, y) x##y
#define qz_concat(x, y) qz_concat_impl(x, y)

#define qz_for_each_rseq_n() 8, 7, 6, 5, 4, 3, 2, 1, 0
#define qz_for_each_argn(_1, _2, _3, _4, _5, _6, _7, _8, n, ...) n
#define qz_for_each_arg_idx_impl(...) qz_for_each_argn(__VA_ARGS__)
#define qz_for_each_arg_idx(...) qz_for_each_arg_idx_impl(__VA_ARGS__ __VA_OPT__(,) qz_for_each_rseq_n())

#define qz_for_each_impl(n, f, ...) qz_concat(qz_for_each_, n)(f, __VA_ARGS__)
#define qz_for_each(f, ...) qz_for_each_impl(qz_for_each_arg_idx(__VA_ARGS__), f, __VA_ARGS__)

#define qz_compare(name) name == rhs.name
#define qz_access_member(name) value.name

#define qz_make_hashable(type, ...)                                                     \
template <>                                                                             \
struct hash<type> {                                                                     \
    qz_nodiscard size_t operator ()(const type& value) const noexcept {                 \
        return qz::util::hash(0, qz_for_each(qz_access_member, __VA_ARGS__));           \
    }                                                                                   \
}

#define qz_make_equal_to(type, ...)                                                 \
qz_nodiscard bool operator ==(const type& rhs) const noexcept {                     \
    return [](auto&&... args) {                                                     \
        return (args && ...);                                                       \
    }(qz_for_each(qz_compare, __VA_ARGS__));                                        \
}


#if _MSVC_LANG >= 201703L || __cplusplus >= 201703L
    #define qz_nodiscard [[nodiscard]]
    #define qz_noreturn [[noreturn]]
#else
    #define qz_nodiscard
    #define qz_noreturn
#endif

#if defined(_WIN32)
    #define qz_unreachable() __assume(false)
#else
    #define qz_unreachable() __builtin_unreachable()
#endif

#if defined(quartz_debug)
    #define qz_assert(expr, msg) assert((expr) && msg)
    #define qz_vulkan_check(expr) qz_assert((expr) == VK_SUCCESS, "Result was not VK_SUCCESS")
#else
    #define qz_assert(expr, msg) (void)(expr), (void)(msg)
    #define qz_vulkan_check(expr) (void)(expr)
#endif

#define qz_force_assert(msg) (qz_assert(false, msg), qz_unreachable())

#define qz_load_instance_function(ctx, name) const auto name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(ctx.instance, #name))
