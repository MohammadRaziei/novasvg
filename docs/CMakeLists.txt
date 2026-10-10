# NovaSVG Documentation Build
# Builds C++ docs (Doxygen) and Python docs (Sphinx) via CreateDocs.cmake scripts,
# plus the CLI reference (Markdown) and the route index page.

cmake_minimum_required(VERSION 3.19)

set(NOVASVG_DOCS_OUT ${CMAKE_CURRENT_BINARY_DIR}/out)

set(NOVASVG_DOCS_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cpp)
set(NOVASVG_DOCS_CPP_OUT ${NOVASVG_DOCS_OUT}/cpp)

set(NOVASVG_DOCS_PYTHON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/python)
set(NOVASVG_DOCS_PYTHON_OUT ${NOVASVG_DOCS_OUT}/python)


# ── C++ Documentation (Doxygen) ───────────────────────────────

find_package(Doxygen)

if(DOXYGEN_FOUND)
    set(NOVASVG_DOCS_CPP_HTML ${NOVASVG_DOCS_CPP_OUT}/html/index.html)

    add_custom_command(
        OUTPUT ${NOVASVG_DOCS_CPP_HTML}
        COMMAND ${CMAKE_COMMAND}
            -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
            -DPROJECT_VERSION=${PROJECT_VERSION}
            -DOUTPUT_DIR=${NOVASVG_DOCS_CPP_OUT}
            -DDOXYGEN_EXECUTABLE=${DOXYGEN_EXECUTABLE}
            -DFOOTER_IN=${NOVASVG_DOCS_CPP_DIR}/footer.html.in
            -DDOXYFILE_IN=${NOVASVG_DOCS_CPP_DIR}/Doxyfile.in
            -DLOGO_SRC=${PROJECT_SOURCE_DIR}/docs/images/novasvg-theme.svg
            -P ${NOVASVG_DOCS_CPP_DIR}/CreateDocs.cmake
        DEPENDS
            ${NOVASVG_DOCS_CPP_DIR}/CreateDocs.cmake
            ${NOVASVG_DOCS_CPP_DIR}/footer.html.in
            ${NOVASVG_DOCS_CPP_DIR}/Doxyfile.in
            ${NOVASVG_DOCS_CPP_DIR}/mainpage.md
        COMMENT "Generating C++ API documentation with Doxygen"
        VERBATIM
    )

    add_custom_target(novasvg_docs_cpp DEPENDS ${NOVASVG_DOCS_CPP_HTML})
else()
    message(WARNING "Doxygen not found - C++ documentation will not be generated")
    add_custom_target(novasvg_docs_cpp
        COMMAND ${CMAKE_COMMAND} -E echo "Skipping C++ docs: Doxygen not found"
    )
endif()


# ── Python Documentation (Sphinx) ─────────────────────────────

find_package(Python3 COMPONENTS Interpreter REQUIRED)

execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import sphinx"
    OUTPUT_QUIET
    ERROR_QUIET
    RESULT_VARIABLE SPHINX_CHECK
)

if(SPHINX_CHECK EQUAL 0)
    set(NOVASVG_DOCS_PYTHON_HTML ${NOVASVG_DOCS_PYTHON_OUT}/html/index.html)

    add_custom_command(
        OUTPUT ${NOVASVG_DOCS_PYTHON_HTML}
        COMMAND ${CMAKE_COMMAND}
            -DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}
            -DSOURCE_DIR=${NOVASVG_DOCS_PYTHON_DIR}/source
            -DOUTPUT_DIR=${NOVASVG_DOCS_PYTHON_OUT}
            -DNOVASVG_PYTHON_INSTALL_DIR=${CMAKE_BINARY_DIR}/python/install
            -P ${NOVASVG_DOCS_PYTHON_DIR}/CreateDocs.cmake
        DEPENDS
            ${NOVASVG_DOCS_PYTHON_DIR}/CreateDocs.cmake
            ${NOVASVG_DOCS_PYTHON_DIR}/source/conf.py
            ${NOVASVG_DOCS_PYTHON_DIR}/source/index.rst
        COMMENT "Building Python API documentation with Sphinx"
        VERBATIM
    )

    add_custom_target(novasvg_docs_python DEPENDS ${NOVASVG_DOCS_PYTHON_HTML})

    # Must have novasvg_build_python run first so the module is installed
    if(TARGET novasvg_build_python)
        add_dependencies(novasvg_docs_python novasvg_build_python)
    endif()
else()
    message(WARNING "Sphinx not found - Python documentation will not be generated")
    add_custom_target(novasvg_docs_python
        COMMAND ${CMAKE_COMMAND} -E echo "Skipping Python docs: Sphinx not found"
    )
endif()


# ── Route Index Page ──────────────────────────────────────────

set(NOVASVG_INDEX_OUT ${NOVASVG_DOCS_OUT}/index.html)

# Per-language tab content for the docs/install/example panels: supported_langs.json
# lists the languages (see its "_comment" field for the format), and each language
# folder holds index-docs.json (structured fields) + index-install.txt +
# index-example.txt (raw HTML). generate_index.py assembles index.html from these.
# Globbed at configure time so adding or editing a fragment triggers a reconfigure —
# new fragment files need `cmake` re-run once (e.g. via `touch supported_langs.json`)
# for the glob itself to pick them up.
set(NOVASVG_SUPPORTED_LANGS ${CMAKE_CURRENT_SOURCE_DIR}/supported_langs.json)
set(NOVASVG_GENERATE_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/generate_index.py)
file(GLOB_RECURSE NOVASVG_LANG_FRAGMENTS
    ${CMAKE_CURRENT_SOURCE_DIR}/*/index-docs.json
    ${CMAKE_CURRENT_SOURCE_DIR}/*/index-install.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/*/index-example.txt
)

add_custom_command(
    OUTPUT ${NOVASVG_INDEX_OUT}
    COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_DIR=${NOVASVG_DOCS_OUT}
        -DINDEX_IN=${CMAKE_CURRENT_SOURCE_DIR}/index.html.in
        -DCSS_FILE=${CMAKE_CURRENT_SOURCE_DIR}/novasvg-docs.css
        -DPROJECT_VERSION=${PROJECT_VERSION}
        -DLOGO_SQ=${PROJECT_SOURCE_DIR}/docs/images/novasvg-sq.svg
        -DLANGS_FILE=${NOVASVG_SUPPORTED_LANGS}
        -DLANGS_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DGENERATE_SCRIPT=${NOVASVG_GENERATE_SCRIPT}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CreateDocs.cmake
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/CreateDocs.cmake
        ${NOVASVG_GENERATE_SCRIPT}
        ${CMAKE_CURRENT_SOURCE_DIR}/index.html.in
        ${CMAKE_CURRENT_SOURCE_DIR}/novasvg-docs.css
        ${PROJECT_SOURCE_DIR}/docs/images/novasvg-sq.svg
        ${NOVASVG_SUPPORTED_LANGS}
        ${NOVASVG_LANG_FRAGMENTS}
    COMMENT "Creating documentation route index"
    VERBATIM
)

add_custom_target(novasvg_docs_index DEPENDS ${NOVASVG_INDEX_OUT})


# ── Markdown pages (NovaSVGPages) ───────────────────────────────

set(NOVASVG_DOCS_SRC ${CMAKE_CURRENT_SOURCE_DIR})

set(LOGO_SQ ${PROJECT_SOURCE_DIR}/docs/images/novasvg-sq.svg)

include(${CMAKE_CURRENT_SOURCE_DIR}/NovaSVGPages.cmake)

# novasvg_add_page(NAME cli        SOURCE cli.md        TITLE "CLI Reference")
# novasvg_add_page(NAME benchmark  SOURCE benchmark.md  TITLE "Benchmarks")
# novasvg_add_page(NAME changelog  SOURCE changelog.md  TITLE "Changelog")

novasvg_add_section(
    NAME  cli
    TITLE "CLI Reference"
    SOURCES
        "cli/index.md:Overview"
        "cli/installation.md:Installation"
        "cli/options.md:Options"
        "cli/examples.md:Examples"
)

# ── Master target ─────────────────────────────────────────────

add_custom_target(novasvg_docs
    COMMAND ${CMAKE_COMMAND} -E echo "-- All documentation generated"
    COMMENT "Building all documentation"
)

add_dependencies(novasvg_docs novasvg_docs_cpp novasvg_docs_python novasvg_docs_index)
novasvg_finalize_pages(novasvg_docs)


# ── Post-process: inject HTML comment into all output HTML files ──

add_custom_command(TARGET novasvg_docs POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_DIR=${NOVASVG_DOCS_OUT}
        -DCOMMENT_IN=${CMAKE_CURRENT_SOURCE_DIR}/initial_comment.in
        -DASCII_ART_FILE=${CMAKE_CURRENT_SOURCE_DIR}/images/novasvg-sq.txt
        -DPROJECT_VERSION=${PROJECT_VERSION}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/InjectComment.cmake
    COMMENT "Injecting HTML comments into documentation pages"
    VERBATIM
)

# ── Benchmarks page ───────────────────────────────────────────
# benchmarks/outputs/report.html is generated on a developer machine
# (see benchmarks/README.md) and committed; it is only copied here, never
# rebuilt. Runs after InjectComment above so the standalone file is
# published byte-for-byte as committed.
set(NOVASVG_BENCH_REPORT ${PROJECT_SOURCE_DIR}/benchmarks/outputs/report.html)
if(EXISTS ${NOVASVG_BENCH_REPORT})
    add_custom_command(TARGET novasvg_docs POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${NOVASVG_DOCS_OUT}/benchmarks
        COMMAND ${CMAKE_COMMAND} -E copy ${NOVASVG_BENCH_REPORT} ${NOVASVG_DOCS_OUT}/benchmarks/index.html
        COMMENT "Publishing committed benchmark report -> ${NOVASVG_DOCS_OUT}/benchmarks/index.html"
        VERBATIM
    )
else()
    message(WARNING "benchmarks/outputs/report.html not found -- the docs site "
                    "will have no /benchmarks/ page. Generate it as described in "
                    "benchmarks/README.md and commit it.")
endif()
