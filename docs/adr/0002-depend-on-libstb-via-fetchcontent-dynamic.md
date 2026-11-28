# ADR-0002: Depend on libstb: fetched with FetchContent, pinned, linked dynamically

**Status:** Accepted and implemented (step 1). The way libstb is obtained is explicitly provisional, see Revisit.
**Date:** 2026-11-01
**Deciders:** Mohammad Raziei (maintainer)

## Context

novasvg embeds copies of code that libstb, a sibling project by the same author, provides as a library:
image decoding (stb_image 2.30, about 7,700 lines), an image encoder (a port of stb_image_write, about 1,400
lines), a zlib inflater, and TrueType support (stb_truetype, about 5,000 lines). libstb also has colour, blend,
resize, font and text-shaping code; its `stb::color` was derived from novasvg's `Color`.

The intent is to use libstb "as much as possible" instead of carrying private copies. That needs a decision on
how novasvg obtains libstb and how it links it.

## Decision

1. libstb is fetched with CMake **`FetchContent` from its git repository**, pinned to a commit
   (`765f48ce476194a98f2a69af95f9711aef06d8a6`, libstb 0.8.0). Repository and pin are cache variables
   (`NOVASVG_LIBSTB_GIT_REPOSITORY`, `NOVASVG_LIBSTB_GIT_TAG`); `FETCHCONTENT_SOURCE_DIR_LIBSTB` points the
   build at a local checkout and `FETCHCONTENT_FULLY_DISCONNECTED=ON` keeps it offline.
2. libstb is **always linked dynamically**. CMake forces `STB_BUILD_SHARED=ON` and stops with an error if
   the resulting target is not a shared library.
3. The dependency hangs off `novasvg::noflag`, so everything that links novasvg gets libstb's include path and
   link line. `novasvgConfig.cmake` calls `find_dependency(libstb)`.

## Options Considered

### How to obtain libstb

| Option | Reproducible | Works offline | Local co-development | Notes |
|---|---|---|---|---|
| `add_subdirectory` of a sibling checkout | Only if the sibling is the right version | Yes | Excellent | Easy to build against the wrong libstb |
| **`FetchContent` from git, pinned (chosen)** | Yes (commit hash) | With `FETCHCONTENT_SOURCE_DIR_LIBSTB` | Good (same variable) | Needs network on first configure |
| `find_package(libstb)` of an installed copy | Depends on the system | Yes | Poor | libstb must be installed first |
| git submodule | Yes | Yes | Good | Extra clone step; submodule friction |
| Keep vendoring | Yes | Yes | n/a | The thing being removed (ADR-0005) |

### Static or shared

| Option | Assessment |
|---|---|
| Static | Simplest to ship, no runtime lookup; duplicates libstb in every consumer that also links it |
| **Shared (chosen)** | One copy of libstb shared by novasvg and other users of it; needs RUNPATH, DLL placement and wheel bundling |

## Trade-off Analysis

FetchContent gives reproducible builds from a single configure command and still allows a local checkout, which
is what the maintainer needs while both projects change together. Shared linking was a maintainer requirement:
it keeps one libstb in a process that uses both projects. Its price is deployment work (below), accepted
deliberately.

## Consequences

- **Easier:** about 9,150 lines of vendored code removed so far (ADR-0005); libstb fixes reach novasvg by
  bumping one pin; one SIMD-dispatching implementation of image/zlib/blend code.
- **Harder / found while implementing:**
  - Both projects have `cmake/Startup.cmake`, `Optimize.cmake`, `DynamicVersion.cmake`, and libstb appends its
    module directory *after* the parent's. Left alone, libstb silently included novasvg's `Optimize.cmake`
    (measured: libstb lost `-fstrict-aliasing`). novasvg clears `CMAKE_MODULE_PATH` while libstb configures.
    Suggested fix in libstb: `list(PREPEND CMAKE_MODULE_PATH ...)`.
  - Installing novasvg also installs libstb (shared library, headers, CMake config) into the same prefix. The
    installed CLI gets `INSTALL_RPATH` `$ORIGIN/../lib` (`@loader_path/../lib` on macOS), or it would not find
    libstb after install. Verified with a separate `find_package(novasvg)` consumer project.
  - Minimum CMake rises from 3.19 to 3.21 (libstb requires it).
  - First configure needs network access unless a local source directory is given.
- **Known risk:** libstb's recent history shows shared-library trouble on Windows (DLL export and placement) and
  on musllinux (a segfault). Those now apply to novasvg too and must be checked when building wheels (step 7).
- **Revisit:** the maintainer chose FetchContent "for now"; how libstb is obtained may change. The pin is
  bumped on purpose, not automatically.

## Action Items

1. [x] FetchContent block, pin, shared enforced, module-path workaround (step 1).
2. [x] `find_dependency(libstb)` in the package config; `INSTALL_RPATH` for the CLI.
3. [x] `test_novasvg_libstb`: links only through `novasvg::noflag` and proves (`dladdr`) that libstb is a
       separate shared object.
4. [ ] Verify Windows DLL placement and musllinux behaviour (step 7).
5. [ ] Propose `list(PREPEND ...)` in libstb's CMake.
6. [ ] Revisit how libstb is obtained once the maintainer's plan for it is settled.
