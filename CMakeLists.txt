cmake_minimum_required(VERSION 3.21)

project(voidwalk
    VERSION 0.1.0
    DESCRIPTION "Binary analysis tool for ELF and PE executables"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The single voidwalk binary dispatches by argv[1]: --gui or --ui/<binary>. Each
# frontend is compiled in only when its option is ON. The test suite is a SEPARATE
# executable (voidwalk-tests) so it builds and runs with neither Qt nor FTXUI - that is
# what CI compiles.
option(VOIDWALK_BUILD_TUI   "Compile the FTXUI terminal frontend into voidwalk" ON)
option(VOIDWALK_BUILD_GUI   "Compile the Qt6 GUI frontend into voidwalk"        ON)
option(VOIDWALK_BUILD_TESTS "Build the standalone voidwalk-tests binary"        ON)

enable_testing()

# Mirror the Visual Studio projects, which all set <OutDir>exec</OutDir>. On
# multi-config generators CMake appends the config name unless each config is
# pinned explicitly.
set(VOIDWALK_EXEC_DIR "${CMAKE_SOURCE_DIR}/exec")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${VOIDWALK_EXEC_DIR}")
foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${config}" config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${VOIDWALK_EXEC_DIR}")
endforeach()

# ---------------------------------------------------------------------------
# voidwalk-core — format parsing shared by every frontend.
# ---------------------------------------------------------------------------

add_library(voidwalk-core STATIC
    main/address-space/address_space.cpp
    main/disassembler/disassembler.cpp
    main/disassembler/ELF/elf_disassembler.cpp
    main/disassembler/PE/pe_disassembler.cpp)

# Sources include each other by relative path; these roots let a frontend reach
# "main/..." and "tests/..." regardless of where its own file sits.
target_include_directories(voidwalk-core PUBLIC
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/main")

# The core (AddressSpace mutex) and every frontend (the decode worker runs on a
# std::jthread) need the platform threading library. Linking it PUBLIC on the core
# propagates to voidwalk and voidwalk-tests. No-op on MSVC; -pthread on GNU/Linux.
find_package(Threads REQUIRED)
target_link_libraries(voidwalk-core PUBLIC Threads::Threads)

# ---------------------------------------------------------------------------
# voidwalk — one executable that bundles every frontend. main/main.cpp routes
# by argv[1]:
#   voidwalk --gui                -> Qt6 GUI     (needs VOIDWALK_BUILD_GUI)
#   voidwalk --ui <bin> | <bin>   -> FTXUI TUI   (needs VOIDWALK_BUILD_TUI)
# ---------------------------------------------------------------------------

add_executable(voidwalk
    main/main.cpp)
# app.rc is a Windows resource script (the app icon). It is only meaningful with an RC
# compiler, so keep it off non-Windows configures - otherwise a Linux/macOS `cmake -B build`
# fails at generation on an unknown RC source, which would break CI before it builds anything.
if(WIN32)
    target_sources(voidwalk PRIVATE main/app.rc)
endif()
target_link_libraries(voidwalk PRIVATE voidwalk-core)

# --- TUI (FTXUI) -----------------------------------------------------------
if(VOIDWALK_BUILD_TUI)
    # Prefer an FTXUI already provided by a package manager (the vcpkg.json
    # manifest pins it). Fall back to fetching the matching tag so a bare
    # `cmake -B build` works with no package manager installed at all.
    find_package(ftxui 6.1.9 QUIET)

    if(NOT ftxui_FOUND)
        message(STATUS "FTXUI not found locally; fetching v6.1.9")
        include(FetchContent)
        FetchContent_Declare(ftxui
            GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
            GIT_TAG v6.1.9
            GIT_SHALLOW TRUE)
        set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(ftxui)
    endif()

    target_sources(voidwalk PRIVATE
        main/TUI/model/session.cpp
        main/TUI/panes/disassembly_pane.cpp
        main/TUI/panes/memory_pane.cpp
        main/TUI/panes/registers_pane.cpp
        main/TUI/panes/stack_pane.cpp
        main/TUI/runner/mainUI.cpp
        main/TUI/ui/ui.cpp)

    # Gates the TUI include + default dispatch in main/main.cpp.
    target_compile_definitions(voidwalk PRIVATE VOIDWALK_WITH_TUI)

    target_link_libraries(voidwalk PRIVATE
        ftxui::screen
        ftxui::dom
        ftxui::component)
endif()

# --- GUI (Qt6) -------------------------------------------------------------
# Off by default: Qt is the one dependency CMake cannot fetch for you. Turn on
# with -DVOIDWALK_BUILD_GUI=ON once qt6-base-dev / qt6-svg-dev are installed.
if(VOIDWALK_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

    # The voidwalk target already exists at this point, so
    # qt_standard_project_setup()'s variable tweaks would arrive too late to
    # affect it. Enable Qt autogen (moc/uic/rcc) on the target directly. AUTORCC
    # is what processes main/GUI/rsrc/mainWindow.qrc; without it the build still
    # links but every toolbar icon (icons.cpp reads ":/icons/<name>.svg") fails
    # to load at runtime.
    set_target_properties(voidwalk PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON)

    target_sources(voidwalk PRIVATE
        main/GUI/runner/gui_main.cpp
        main/GUI/model/gui_session.cpp
        main/GUI/model/symbols.cpp
        main/GUI/ai/ai_client.cpp
        main/GUI/dialogs/settings_dialog.cpp
        main/GUI/panes/ai_chat_pane.cpp
        main/GUI/panes/column_fit.cpp
        main/GUI/panes/disasm_delegate.cpp
        main/GUI/panes/disassembly_pane.cpp
        main/GUI/panes/memory_pane.cpp
        main/GUI/panes/registers_pane.cpp
        main/GUI/panes/stack_pane.cpp
        main/GUI/panes/symbols_pane.cpp
        main/GUI/theme/icons.cpp
        main/GUI/theme/theme.cpp
        main/GUI/theme/theme_manager.cpp
        main/GUI/widgets/main_window.cpp
        main/GUI/widgets/welcome_widget.cpp
        main/GUI/rsrc/mainWindow.qrc)

    # main/GUI/widgets/mainWindow.{h,cpp}, main/GUI/runner/mainGUI.cpp and
    # main/GUI/ui/mainWindow.ui are the superseded first pass - left out
    # deliberately; do not add them back without reviving the .ui.

    # Gates the GUI include + "--gui" dispatch in main/main.cpp.
    target_compile_definitions(voidwalk PRIVATE VOIDWALK_WITH_GUI)

    target_link_libraries(voidwalk PRIVATE
        Qt6::Widgets
        Qt6::Svg)

    # NOTE: intentionally NOT WIN32_EXECUTABLE. The bundled binary is a console
    # app so the TUI and test modes have a console; the trade-off is that on
    # Windows the --gui mode also gets a console window behind it.
endif()

# ---------------------------------------------------------------------------
# voidwalk-tests — the test runner. Links ONLY voidwalk-core (no Qt, no FTXUI),
# so `cmake -B build -DVOIDWALK_BUILD_GUI=OFF -DVOIDWALK_BUILD_TUI=OFF` builds
# and runs it on a bare CI machine. The suites synthesise their own ELF/PE/code
# fixtures at runtime, so nothing under .testing/ is needed.
#
#   cmake --build build --target voidwalk-tests
#   ctest --test-dir build --output-on-failure
# ---------------------------------------------------------------------------
if(VOIDWALK_BUILD_TESTS)
    add_executable(voidwalk-tests
        tests/test_main.cpp
        tests/runner.cpp)
    target_link_libraries(voidwalk-tests PRIVATE voidwalk-core)

    add_test(NAME voidwalk-tests COMMAND voidwalk-tests)
endif()

# ---------------------------------------------------------------------------
# Sanitizers.
#
# The robustness suite asks "does this malformed binary crash us?", and a plain
# build can only answer that for faults that actually fault - reading
# instructionBytes[-1] reads adjacent stack and looks like a pass. Under ASan/UBSan
# the same input aborts and the check fails, which is the point.
#
#   cmake -B build -DVOIDWALK_SANITIZE=ON -DVOIDWALK_BUILD_GUI=OFF -DVOIDWALK_BUILD_TUI=OFF
#   cmake --build build --target voidwalk-tests
#   ./exec/voidwalk-tests
#
# -fno-sanitize-recover=all is not optional here: UBSan otherwise prints a diagnostic
# and continues, so an out-of-bounds index lands in the log while the check still
# passes. With it, detected UB aborts and the suite sees the failure.
#
# MSVC has /fsanitize=address but no UBSan and no -fno-sanitize-recover equivalent,
# so ASan alone is applied there.
# ---------------------------------------------------------------------------
option(VOIDWALK_SANITIZE "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)

if(VOIDWALK_SANITIZE)
    if(MSVC)
        set(VOIDWALK_SAN_FLAGS /fsanitize=address)
        message(STATUS "Sanitizers: ASan (MSVC has no UBSan)")
    else()
        set(VOIDWALK_SAN_FLAGS
            -fsanitize=address,undefined
            -fno-sanitize-recover=all
            -fno-omit-frame-pointer)
        message(STATUS "Sanitizers: ASan + UBSan, halting on first error")
    endif()

    # PUBLIC on the core: both voidwalk and voidwalk-tests link it, so the flags reach
    # every target that matters without being repeated. Sanitizers must be applied at
    # compile AND link time, hence both calls.
    target_compile_options(voidwalk-core PUBLIC ${VOIDWALK_SAN_FLAGS})
    target_link_options(voidwalk-core PUBLIC ${VOIDWALK_SAN_FLAGS})
endif()

# ---------------------------------------------------------------------------
# Warnings.
#
# On by default rather than opt-in: several AUDIT.md findings are diagnosed by the
# compiler for free - the discarded std::length_error (-Wunused-value), the varargs
# type mismatch in the immediate-size fallback (-Wformat), the malformed bounds-check
# line in readType (-Wmisleading-indentation), and the by-value PE_Sections parameter
# (-Wunused-parameter).
#
# -Wmissing-field-initializers is turned back OFF deliberately. The opcode tables build
# ~4,500 OpcodeInfo/TableOperand aggregates through the P()/OP() macros, each leaving
# trailing members to be value-initialised on purpose. That produces 4,554 warnings
# against about 70 real ones - a ratio at which nobody reads the output and the whole
# flag gets switched off. Suppressing the one noisy category is what keeps the rest
# usable.
#
# Applied to every target, not just the core: main.cpp, the TUI and the GUI are where
# most of the code lives. Qt and FTXUI arrive through imported targets whose includes
# are SYSTEM, so their headers stay quiet.
# ---------------------------------------------------------------------------
if(MSVC)
    set(VOIDWALK_WARN_FLAGS /W4)
else()
    set(VOIDWALK_WARN_FLAGS -Wall -Wextra -Wno-missing-field-initializers)
endif()

foreach(voidwalk_target voidwalk-core voidwalk voidwalk-tests)
    if(TARGET ${voidwalk_target})
        target_compile_options(${voidwalk_target} PRIVATE ${VOIDWALK_WARN_FLAGS})
    endif()
endforeach()

# Opt-in: make the build fail on a warning. Off by default so a new compiler version
# introducing a new diagnostic cannot break a local build, but worth turning on in CI.
option(VOIDWALK_WERROR "Treat compiler warnings as errors" OFF)
if(VOIDWALK_WERROR)
    foreach(voidwalk_target voidwalk-core voidwalk voidwalk-tests)
        if(TARGET ${voidwalk_target})
            target_compile_options(${voidwalk_target} PRIVATE
                $<IF:$<BOOL:${MSVC}>,/WX,-Werror>)
        endif()
    endforeach()
endif()
