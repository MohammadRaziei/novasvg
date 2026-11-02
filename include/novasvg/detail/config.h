#pragma once

// Build configuration shared by every novasvg header: linkage/export macros and the small
// min/max helpers. Kept in its own header so internal headers (and the compiled sources in src/)
// can use them without pulling in the whole public API.

// True header-only linkage: every function/method defined in the detail/
// implementation headers is marked NOVASVG_INLINE (plain `inline`), so
// this whole library -- including its rendering backend -- can safely be
// #included from any number of translation units with no separate
// "#define NOVASVG_IMPLEMENTATION in exactly one .cpp" step required.
#ifndef NOVASVG_INLINE
#define NOVASVG_INLINE inline
#endif

// NovaSVG is header-only: every function is NOVASVG_INLINE, so the default
// linkage must NOT be dllimport/dllexport. On MSVC, __declspec(dllimport) on
// an inline-defined function suppresses the inline body and produces LNK2019
// unresolved externals. Shared-library decoration is therefore opt-in via
// NOVASVG_BUILD_SHARED.
#if defined(NOVASVG_BUILD_STATIC) || !defined(NOVASVG_BUILD_SHARED)
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

// Plain ternary macros instead of std::max/std::min. A uniquely-prefixed
// macro can never collide with anything, which a bare `std::max`/
// `std::min` call genuinely can: <windows.h> (pulled in transitively by
// Python.h and friends on a Windows build) `#define`s `max`/`min` as
// object-like macros unless NOMINMAX is defined first, and since that's
// a build-system concern novasvg can't force on every consumer of a
// header-only library, this codebase calls these instead of std::max/
// std::min directly wherever it needs a two-argument max/min. Same
// pattern the vendored FreeType rasterizer already uses for its own
// internals (PVG_FT_MIN/PVG_FT_MAX in detail/render/vendor/), just
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
