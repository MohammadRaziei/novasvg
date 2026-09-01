#pragma once

// Shared build config: linkage macros, version numbers, and the couple of
// C-style typedefs (novasvg_destroy_func_t/novasvg_write_func_t) that
// appear in several public headers' signatures. Every top-level novasvg
// header (color.h, bitmap.h, font.h, canvas.h, novasvg.h, ...) includes
// this first so they can each be used on their own, not just through the
// novasvg.h umbrella.

#include <cstddef>

// True header-only linkage: every function/method defined across novasvg's
// headers is marked NOVASVG_INLINE (plain `inline`), so the whole library
// -- including its rendering backend -- can safely be #included from any
// number of translation units with no separate "#define NOVASVG_IMPLEMENTATION
// in exactly one .cpp" step required.
#ifndef NOVASVG_INLINE
#define NOVASVG_INLINE inline
#endif

#if defined(NOVASVG_BUILD_STATIC)
#define NOVASVG_EXPORT
#define NOVASVG_IMPORT
#elif (defined(_WIN32) || defined(__CYGWIN__))
#define NOVASVG_EXPORT __declspec(dllexport)
#define NOVASVG_IMPORT __declspec(dllimport)
#elif defined(__GNUC__) && (__GNUC__ >= 4)
#define NOVASVG_EXPORT __attribute__((__visibility__("default")))
#define NOVASVG_IMPORT
#else
#define NOVASVG_EXPORT
#define NOVASVG_IMPORT
#endif

#ifdef NOVASVG_BUILD
#define NOVASVG_API NOVASVG_EXPORT
#else
#define NOVASVG_API NOVASVG_IMPORT
#endif

#define NOVASVG_VERSION_MAJOR 0
#define NOVASVG_VERSION_MINOR 3
#define NOVASVG_VERSION_PATCH 0

#define NOVASVG_VERSION_ENCODE(major, minor, patch) (((major) * 10000) + ((minor) * 100) + ((patch) * 1))
#define NOVASVG_VERSION NOVASVG_VERSION_ENCODE(NOVASVG_VERSION_MAJOR, NOVASVG_VERSION_MINOR, NOVASVG_VERSION_PATCH)

#define NOVASVG_VERSION_XSTRINGIZE(major, minor, patch) #major"."#minor"."#patch
#define NOVASVG_VERSION_STRINGIZE(major, minor, patch) NOVASVG_VERSION_XSTRINGIZE(major, minor, patch)
#define NOVASVG_VERSION_STRING NOVASVG_VERSION_STRINGIZE(NOVASVG_VERSION_MAJOR, NOVASVG_VERSION_MINOR, NOVASVG_VERSION_PATCH)

/**
 * @brief Callback for cleaning up resources.
 *
 * This function is called to release resources associated with a specific operation.
 *
 * @param closure A user-defined pointer to the resource or context to be freed.
 */
typedef void (*novasvg_destroy_func_t)(void* closure);

/**
 * @brief A function pointer type for a write callback.
 * @param closure A pointer to user-defined data or context.
 * @param data A pointer to the data to be written.
 * @param size The size of the data in bytes.
 */
typedef void (*novasvg_write_func_t)(void* closure, void* data, int size);
