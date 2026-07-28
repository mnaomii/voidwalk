cmake_minimum_required(VERSION 3.21)

project(voidwalk
    VERSION 0.1.0
    DESCRIPTION "Binary analysis tool for ELF and PE executables"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The single voidwalk binary dispatches by argv[1]: --gui, --ui/<binary>, or
# --run-tests. Each frontend is compiled in only when its option is ON; the test
# harness is always bundled (it is tiny and main.cpp always routes --run-tests).
option(VOIDWALK_BUILD_TUI "Compile the FTXUI terminal frontend into voidwalk" ON)
option(VOIDWALK_BUILD_GUI "Compile the Qt6 GUI frontend into voidwalk"        ON)

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

# ---------------------------------------------------------------------------
# voidwalk — one executable that bundles every frontend. main/main.cpp routes
# by argv[1]:
#   voidwalk --gui                -> Qt6 GUI     (needs VOIDWALK_BUILD_GUI)
#   voidwalk --ui <bin> | <bin>   -> FTXUI TUI   (needs VOIDWALK_BUILD_TUI)
#   voidwalk --run-tests <bin>    -> test harness (always compiled in)
# ---------------------------------------------------------------------------

add_executable(voidwalk
    main/main.cpp
    tests/runner.cpp)
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
        main/GUI/ai/ai_client.cpp
        main/GUI/dialogs/settings_dialog.cpp
        main/GUI/panes/ai_chat_pane.cpp
        main/GUI/panes/disasm_delegate.cpp
        main/GUI/panes/disassembly_pane.cpp
        main/GUI/panes/memory_pane.cpp
        main/GUI/panes/registers_pane.cpp
        main/GUI/panes/stack_pane.cpp
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
