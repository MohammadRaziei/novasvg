#pragma once

// Build configuration shared by every novasvg header: linkage/export macros and the small
// min/max helpers. Kept in its own header so internal headers (and the compiled sources in src/)
// can use them without pulling in the whole public API.

// Linkage: NOVASVG_INLINE marks the definitions that have not moved out of the headers yet (step 4 of
// the refactor). NOVASVG_API marks what the compiled library exports. Same scheme as STB_API in libstb:
//
//   building the library        NOVASVG_BUILDING_LIBRARY is defined: dllexport on Windows, default
//                               visibility elsewhere.
//   using the library           dllimport on Windows, default visibility elsewhere.
//   static library              NOVASVG_STATIC_DEFINE is defined (CMake does it for a static build):
//                               NOVASVG_API is empty.
#ifndef NOVASVG_INLINE
#define NOVASVG_INLINE inline
#endif

#if defined(NOVASVG_STATIC_DEFINE)
#define NOVASVG_API
#elif defined(_WIN32) || defined(__CYGWIN__)
#if defined(NOVASVG_BUILDING_LIBRARY)
#define NOVASVG_API __declspec(dllexport)
#else
#define NOVASVG_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define NOVASVG_API __attribute__((visibility("default")))
#else
#define NOVASVG_API
#endif

// Plain ternary macros instead of std::max/std::min. A uniquely-prefixed
// macro can never collide with anything, which a bare `std::max`/
// `std::min` call genuinely can: <windows.h> (pulled in transitively by
// Python.h and friends on a Windows build) `#define`s `max`/`min` as
// object-like macros unless NOMINMAX is defined first, and since that's
// a build-system concern novasvg can't force on every consumer of a
// header-only library, this codebase calls these instead of std::max/
// std::min directly wherever it needs a two-argument max/min. Same
// pattern the vendored FreeType rasterizer already uses for its own
// internals (PVG_FT_MIN/PVG_FT_MAX in src/render/vendor/), just
// under this project's own prefix for general-purpose call sites.
// ponytail: no protection against double-evaluating a/b if either has
// side effects -- fine for every current call site (plain variable/
// member reads), just don't reach for these with e.g. `x++` as an
// argument.
#ifndef NOVASVG_MAX
#define NOVASVG_MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef NOVASVG_MIN
#define NOVASVG_MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
