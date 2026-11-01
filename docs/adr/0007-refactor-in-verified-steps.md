# ADR-0007: Refactor in verified steps, gated by tests and a pixel-level golden diff

**Status:** Accepted
**Date:** 2026-11-01
**Deciders:** Mohammad Raziei (maintainer)

## Context

The 0.12 refactor changes the build system, replaces dependencies, changes the public image type, renames the
whole API and rearranges the code (ADR-0001 to ADR-0006). Together that is a rewrite of the surface of a
renderer whose correctness is *visual*: a unit test can pass while a gradient edge moves by one level.

Constraints: the work happens on a machine with one core, where a full rebuild of the test targets takes
several minutes, and results are delivered as a zip after each step, to be pushed on a new branch by the maintainer.

## Decision

Work in small steps. A step counts as done only when all of the following hold:

1. The project builds.
2. The unit tests match or beat the baseline: `test_novasvg_cpp` 64 cases / 377 assertions (plus any tests added
   since), `test_novasvg_embedded_font` 5 / 71, and `test_novasvg_libstb`.
3. `tools/golden.py diff` against the golden renders reports **zero differing pixels** (tolerance 0), or each
   difference is explained in the checklist.
4. `REFACTOR_CHECKLIST.md` is updated with what was verified and what was found.
5. A zip of the project is delivered.

The golden renders are the 62 PNGs produced by the baseline CLI (commit `85ebfc7`, version 0.11.4) from every
SVG under `data/`. They live in `refactor/golden-baseline/`, which is listed in `.gitignore`, so they travel in
the zip but are never committed. They can be regenerated from the baseline commit (the checklist has the
commands). `golden.py` decodes PNGs before comparing, so changing the PNG encoder does not count as a
difference. It was tested both ways: two runs of one binary give zero differences, and a deliberate one-level
change in one pixel is reported.

Where a step touches code the golden set does not exercise, the gap is closed first with a test or by
extending the set, not by trusting the step (ADR-0005 did this for the image writers).

## Options Considered

| Option | Assessment |
|---|---|
| Large rewrite, then fix whatever broke | Fewer builds, but the cause of any failure is unknown |
| Unit tests only | Cheap, but blind to small pixel changes |
| **Small steps with unit tests and a pixel gate (chosen)** | More builds and delivery overhead; each failure has one suspect |
| Commit golden images to the repository | Reproducible for everyone, but 3.2 MB of binaries in history; rejected by the maintainer |

## Trade-off Analysis

The refactor's main risk is silent visual change, and the pixel gate is the only check that sees it. It costs a
render of 62 files per step. Keeping the images untracked avoids bloating the repository; regenerating from a
named commit keeps them reproducible.

## Consequences

- **Easier:** every step has a clear pass/fail and a bounded suspect; regressions are caught the step they are
  introduced.
- **Harder:** each step carries the cost of a full build and test run on slow hardware, and of writing up the
  result.
- **Limits:** the golden set covers the 62 SVGs under `data/`, so a feature none of them uses is unprotected;
  the Python tests are not part of the gate yet (the bindings need nanobind and are built in step 7).
- **Revisit:** extend the golden set whenever a step touches something it does not cover (text with RTL,
  embedded JPEG/GIF images, `mix-blend-mode`, filters).

## Action Items

1. [x] `tools/golden.py` (render and diff) and the baseline renders.
2. [x] Per-step checklist with the exit criteria above.
3. [ ] Add RTL text, JPEG/GIF images, `mix-blend-mode` and filter SVGs to the golden set (before steps 2b, 3, 5).
4. [ ] Bring the Python tests into the gate in step 7.
