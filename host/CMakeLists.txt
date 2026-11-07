# SPDX-License-Identifier: GPL-3.0-only
cmake_minimum_required(VERSION 3.22)
project(PaleotoneInstrumentHost LANGUAGES C CXX)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(CR_PYTHON "${Python3_EXECUTABLE}" CACHE FILEPATH "Firmware generator Python")
set(FM1VST_DIR "" CACHE PATH "Pinned FM1VST checkout")
set(REMOTE_ANDROID_DIR "" CACHE PATH "Pinned RemoteFM1Android checkout")
get_filename_component(PALEOTONE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
if(NOT EXISTS "${FM1VST_DIR}/cmake/FM1Cores.cmake")
    message(FATAL_ERROR "Set FM1VST_DIR to the pinned RemoteFM1Android/vendor/FM1VST checkout")
endif()
include("${FM1VST_DIR}/cmake/FM1Cores.cmake")
set(MATH_LIB m)
if(ANDROID)
    if(NOT ANDROID_ABI STREQUAL "arm64-v8a")
        message(FATAL_ERROR "The selected RemoteFM1 Android host currently supports arm64-v8a only")
    endif()
    list(REMOVE_ITEM FM1_COMMON_SOURCES "${FM1_COMMON_DIR}/coro.c")
    list(APPEND FM1_COMMON_SOURCES "${REMOTE_ANDROID_DIR}/cores/android/coro_arm64.c")
endif()
file(GLOB PALEOTONE_NOTICES "${PALEOTONE_DIR}/LICENSES/*.txt")
fm1_add_core(paleotone "${CMAKE_CURRENT_LIST_DIR}"
    SOURCE "${PALEOTONE_DIR}" UNIT core_paleotone.c DESC core_paleotone_desc.c
    FAMILY felucca_web LICENSE GPL-3.0-only VERSION "0.1.1-host-prototype"
    INCLUDES "${FM1VST_DIR}/core-glue/felucca_web"
    DEFINES FM1CORE_FW_WEB_C="${PALEOTONE_DIR}/web/emu/felucca_web.c"
    FLAGS -fwrapv -ffp-contract=off
    GEN ui_fonts.h ui_icons.h ui_keycaps.h ui_palettes.h felucca_tables.h felucca_fm6.h felucca_samples.h
    CREDITS "Paleotone by Dylan Spaniel, based on Felucca and its contributors. Host adapter uses Quixotic7/FM1VST."
    CREDITS_FILES "${PALEOTONE_DIR}/LICENSING.md" ${PALEOTONE_NOTICES})

# Preserve upstream glue and source layout, without importing its full desktop
# project's test targets (which depend on that project's engine/test libraries).
file(GLOB FELUCCA_NOTICES "${FM1VST_DIR}/cores/Felucca/LICENSES/*.txt")
fm1_add_core(felucca "${FM1VST_DIR}/core-glue/felucca_web"
    SOURCE "${FM1VST_DIR}/cores/Felucca" UNIT core_felucca.c DESC core_felucca_desc.c
    FAMILY felucca_web LICENSE GPL-3.0-only VERSION "1.6.1" SUBMODULES_RECURSIVE
    GEN ui_fonts.h ui_icons.h ui_keycaps.h ui_palettes.h felucca_tables.h felucca_fm6.h felucca_samples.h
    DEFINES FELUCCA_VERSION="v1.6.1"
    CREDITS_FILES "${FM1VST_DIR}/cores/Felucca/LICENSING.md" ${FELUCCA_NOTICES})

file(WRITE "${CMAKE_BINARY_DIR}/exports.map" "{ global: fm1core_get; local: *; };\n")
foreach(id paleotone felucca)
    if(NOT APPLE)
        set_property(TARGET ${id}_core PROPERTY LINK_OPTIONS "-Wl,--version-script=${CMAKE_BINARY_DIR}/exports.map")
    endif()
    if(ANDROID)
        set_target_properties(${id}_core PROPERTIES PREFIX "libfm1core_" SUFFIX ".so")
        target_link_options(${id}_core PRIVATE -Wl,-z,max-page-size=16384)
        target_link_libraries(${id}_core PRIVATE log)
    endif()
endforeach()
if(NOT ANDROID)
    enable_testing()
    add_executable(host_core_integration tests/core_integration.cpp)
    target_include_directories(host_core_integration PRIVATE "${FM1VST_DIR}/core-api")
    target_compile_features(host_core_integration PRIVATE cxx_std_17)
    target_link_libraries(host_core_integration PRIVATE ${CMAKE_DL_LIBS})
    add_test(NAME two_real_instruments COMMAND host_core_integration
        $<TARGET_FILE:paleotone_core> $<TARGET_FILE:felucca_core>)
    # Compare the new module against the original unchanged browser/native unit.
    add_library(paleotone_reference STATIC "${FM1VST_DIR}/core-glue/felucca_web/fw_ref.c")
    target_compile_definitions(paleotone_reference PRIVATE FW_REF_WEB_C="${PALEOTONE_DIR}/web/emu/felucca_web.c")
    target_include_directories(paleotone_reference PRIVATE "${PALEOTONE_DIR}/build/gen" "${PALEOTONE_DIR}/firmware/src")
    target_compile_options(paleotone_reference PRIVATE -O2 -w -fwrapv -ffp-contract=off)
    add_dependencies(paleotone_reference gen_paleotone)
    add_executable(host_audio_fidelity tests/audio_fidelity.cpp)
    target_include_directories(host_audio_fidelity PRIVATE "${FM1VST_DIR}/core-api")
    target_compile_features(host_audio_fidelity PRIVATE cxx_std_17)
    target_link_libraries(host_audio_fidelity PRIVATE paleotone_reference ${CMAKE_DL_LIBS} m)
    add_test(NAME paleotone_original_audio_and_screen COMMAND host_audio_fidelity $<TARGET_FILE:paleotone_core>)
    add_executable(host_preset_audition tests/preset_audition.c)
    target_include_directories(host_preset_audition PRIVATE "${PALEOTONE_DIR}/build/gen" "${PALEOTONE_DIR}/firmware/src")
    target_compile_options(host_preset_audition PRIVATE -O2 -w -fwrapv -ffp-contract=off)
    target_link_libraries(host_preset_audition PRIVATE m)
    add_dependencies(host_preset_audition gen_paleotone)
    add_test(NAME paleotone_held_key_preset_audition COMMAND host_preset_audition)
endif()
