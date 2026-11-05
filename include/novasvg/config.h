#pragma once

#include <stb/export.hpp> // STB_MIN / STB_MAX

// Build configuration shared by every novasvg header: linkage/export macros and the small
// min/max helpers. Kept in its own header so internal headers (and the compiled sources in src/)
// can use them without pulling in the whole public API.

// Linkage: NOVA_INLINE marks the definitions that have not moved out of the headers yet (step 4 of
// the refactor). NOVA_API marks what the compiled library exports. Same scheme as STB_API in libstb:
//
//   building the library        NOVA_BUILDING_LIBRARY is defined: dllexport on Windows, default
//                               visibility elsewhere.
//   using the library           dllimport on Windows, default visibility elsewhere.
//   static library              NOVA_STATIC_DEFINE is defined (CMake does it for a static build):
//                               NOVA_API is empty.
#ifndef NOVA_INLINE
#define NOVA_INLINE inline
#endif

#if defined(NOVA_STATIC_DEFINE)
#define NOVA_API
#elif defined(_WIN32) || defined(__CYGWIN__)
#if defined(NOVA_BUILDING_LIBRARY)
#define NOVA_API __declspec(dllexport)
#else
#define NOVA_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define NOVA_API __attribute__((visibility("default")))
#else
#define NOVA_API
#endif

// Plain ternary macros instead of std::max/std::min. A uniquely-prefixed
// macro can never collide with anything, which a bare `std::max`/
// `std::min` call genuinely can: <windows.h> (pulled in transitively by
// Python.h and friends on a Windows build) `#define`s `max`/`min` as
// object-like macros unless NOMINMAX is defined first, and since that's
// a build-system concern novasvg can't force on every consumer of a
// header-only library, this codebase calls these instead of std::max/
// std::min directly wherever it needs a two-argument max/min. Same
// pattern libstb uses for its own internals (STB_MIN/STB_MAX in
// stb/export.hpp), just
// under this project's own prefix for general-purpose call sites.

/** Callback that frees a user-supplied memory buffer once novasvg no longer needs it. */
typedef void (*novasvg_destroy_func_t)(void* closure);
