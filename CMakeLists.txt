cmake_minimum_required(VERSION 3.19)

# build VST 2?
option(JAMBA_ENABLE_VST2 "Use VST2" ON)

# build Audio Unit?
option(JAMBA_ENABLE_AUDIO_UNIT "Enable Audio Unit" ON)

# Sets the deployment target for macOS
set(JAMBA_MACOS_DEPLOYMENT_TARGET "10.14" CACHE STRING "macOS deployment target")

# To use local jamba install, uncomment the following line (no download)
set(JAMBA_ROOT_DIR "")
#set(JAMBA_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../pongasoft/jamba")

# download jamba framework
include(fetch_jamba.cmake)

# Determine proper architecture for the project
include("${JAMBA_ROOT_DIR}/cmake/JambaSetArchitecture.cmake")

set(PLUGIN_MAJOR_VERSION 1)
set(PLUGIN_MINOR_VERSION 4)
set(PLUGIN_PATCH_VERSION 3)
set(PLUGIN_VERSION "${PLUGIN_MAJOR_VERSION}.${PLUGIN_MINOR_VERSION}.${PLUGIN_PATCH_VERSION}")

project("vst-sam-spl-64" VERSION "${PLUGIN_VERSION}")

# Optional: To use local libsndfile (no download), provide path to where it is located
set(LIBSNDFILE_ROOT_DIR "")
#set(LIBSNDFILE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../erikd/libsndfile")

# Optional: To use local r8brain-free-src (no download), provide path to where it is located
set(R8BRAIN-FREE-SRC_ROOT_DIR "")
#set(R8BRAIN-FREE-SRC_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../pongasoft/r8brain-free-src")

# including those before VST to avoid warning messages
include(libsndfile.cmake)
include(r8brain-free-src.cmake)

# Including miniaudio (for MP3/Flac/Vorbis)
add_subdirectory(external/miniaudio)

# To use local googletest install, uncomment the following line (no download) and modify the path accordingly
set(GOOGLETEST_ROOT_DIR "")
#set(GOOGLETEST_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../google/googletest")

# Include Jamba
include("${JAMBA_ROOT_DIR}/jamba.cmake")

# Location of the source files
set(CPP_SOURCES src/cpp)

# Generating the version.h header file which contains the plugin version (to make sure it is in sync with the version
# defined here)
set(VERSION_DIR "${CMAKE_BINARY_DIR}/generated")
configure_file("${CPP_SOURCES}/version.h.in" "${VERSION_DIR}/version.h")

# Source files for VST2
if (JAMBA_ENABLE_VST2)
  set(vst2_sources
      ${CPP_SOURCES}/SampleSplitter_VST2.cpp
      )
endif ()

# Source files for VST3
set(vst_sources
    ${CPP_SOURCES}/SampleSplitter_VST3.cpp

    ${CPP_SOURCES}/FilePath.h
    ${CPP_SOURCES}/FilePath.cpp
    ${CPP_SOURCES}/Model.h
    ${CPP_SOURCES}/Plugin.h
    ${CPP_SOURCES}/Plugin.cpp
    ${CPP_SOURCES}/SampleBuffers.h
    ${CPP_SOURCES}/SampleBuffers.hpp
    ${CPP_SOURCES}/Sampler.h
    ${CPP_SOURCES}/Sampler.hpp
    ${CPP_SOURCES}/SampleSlice.hpp
    ${CPP_SOURCES}/SampleSlices.hpp
    ${CPP_SOURCES}/SampleSplitterCIDs.h
    ${CPP_SOURCES}/SampleStorage.h
    ${CPP_SOURCES}/SharedSampleBuffersMgr.h
    ${CPP_SOURCES}/Slicer.hpp

    ${CPP_SOURCES}/RT/SampleSplitterProcessor.h
    ${CPP_SOURCES}/RT/SampleSplitterProcessor.cpp

    ${CPP_SOURCES}/GUI/CurrentSample.h
    ${CPP_SOURCES}/GUI/LargeFileDialogController.h
    ${CPP_SOURCES}/GUI/LargeFileDialogController.cpp
    ${CPP_SOURCES}/GUI/OffsettedSliceSettingView.cpp
    ${CPP_SOURCES}/GUI/PadKeyView.cpp
    ${CPP_SOURCES}/GUI/PadController.h
    ${CPP_SOURCES}/GUI/PadController.cpp
    ${CPP_SOURCES}/GUI/PadView.h
    ${CPP_SOURCES}/GUI/PadView.cpp
    ${CPP_SOURCES}/GUI/SampleDisplayView.h
    ${CPP_SOURCES}/GUI/SampleDisplayView.cpp
    ${CPP_SOURCES}/GUI/SampleEditView.h
    ${CPP_SOURCES}/GUI/SampleEditView.cpp
    ${CPP_SOURCES}/GUI/SampleFile.h
    ${CPP_SOURCES}/GUI/SampleFile.cpp
    ${CPP_SOURCES}/GUI/SampleEditScrollbarView.h
    ${CPP_SOURCES}/GUI/SampleEditScrollbarView.cpp
    ${CPP_SOURCES}/GUI/SampleEditController.h
    ${CPP_SOURCES}/GUI/SampleEditController.cpp
    ${CPP_SOURCES}/GUI/SampleInfoView.h
    ${CPP_SOURCES}/GUI/SampleInfoView.cpp
    ${CPP_SOURCES}/GUI/SampleLoaderView.h
    ${CPP_SOURCES}/GUI/SampleLoaderView.cpp
    ${CPP_SOURCES}/GUI/SampleMgr.h
    ${CPP_SOURCES}/GUI/SampleMgr.cpp
    ${CPP_SOURCES}/GUI/SampleOverviewView.cpp
    ${CPP_SOURCES}/GUI/SampleSaverView.cpp
    ${CPP_SOURCES}/GUI/SampleSplitterController.cpp
    ${CPP_SOURCES}/GUI/SamplingButtonView.cpp
    ${CPP_SOURCES}/GUI/SamplingStateView.cpp
    ${CPP_SOURCES}/GUI/SliceSettingView.h
    ${CPP_SOURCES}/GUI/SliceSettingView.cpp
    ${CPP_SOURCES}/GUI/SelectedSliceSettingView.cpp
    ${CPP_SOURCES}/GUI/SliceNumberView.cpp
    ${CPP_SOURCES}/GUI/SlicesActionViews.cpp
    ${CPP_SOURCES}/GUI/Waveform.h
    ${CPP_SOURCES}/GUI/Waveform.cpp
    ${CPP_SOURCES}/GUI/WaveformView.h
    ${CPP_SOURCES}/GUI/WaveformView.cpp
    )

#-------------------------------------------------------------------------------
# Compiler options
#-------------------------------------------------------------------------------
if(WIN32)
  message(STATUS "Adding compiler options")
  # suppress warning about std::copy using unsafe iterators
  set(PLUGIN_COMPILE_OPTIONS "/D_SCL_SECURE_NO_WARNINGS")
endif()

# Location of resources
set(RES_DIR "${CMAKE_CURRENT_LIST_DIR}/resource")

# List of resources (images)
set(vst_resources
    "${RES_DIR}/action_cancel.png"
    "${RES_DIR}/action_cancel_2x.png"
    "${RES_DIR}/action_clear.png"
    "${RES_DIR}/action_clear_2x.png"
    "${RES_DIR}/action_crop.png"
    "${RES_DIR}/action_crop_2x.png"
    "${RES_DIR}/action_cut.png"
    "${RES_DIR}/action_cut_2x.png"
    "${RES_DIR}/action_export.png"
    "${RES_DIR}/action_export_2x.png"
    "${RES_DIR}/action_load.png"
    "${RES_DIR}/action_load_2x.png"
    "${RES_DIR}/action_match.png"
    "${RES_DIR}/action_match_2x.png"
    "${RES_DIR}/action_norm_0dB.png"
    "${RES_DIR}/action_norm_0dB_2x.png"
    "${RES_DIR}/action_norm_3dB.png"
    "${RES_DIR}/action_norm_3dB_2x.png"
    "${RES_DIR}/action_norm_6dB.png"
    "${RES_DIR}/action_norm_6dB_2x.png"
    "${RES_DIR}/action_ok.png"
    "${RES_DIR}/action_ok_2x.png"
    "${RES_DIR}/action_redo.png"
    "${RES_DIR}/action_redo_2x.png"
    "${RES_DIR}/action_reset.png"
    "${RES_DIR}/action_reset_2x.png"
    "${RES_DIR}/action_sample.png"
    "${RES_DIR}/action_sample_2x.png"
    "${RES_DIR}/action_switch_edit_edit_tab.png"
    "${RES_DIR}/action_switch_edit_edit_tab_2x.png"
    "${RES_DIR}/action_switch_edit_io_tab.png"
    "${RES_DIR}/action_switch_edit_io_tab_2x.png"
    "${RES_DIR}/action_switch_edit_sample_tab.png"
    "${RES_DIR}/action_switch_edit_sample_tab_2x.png"
    "${RES_DIR}/action_toggle_slice_loop_all.png"
    "${RES_DIR}/action_toggle_slice_loop_all_2x.png"
    "${RES_DIR}/action_trim.png"
    "${RES_DIR}/action_trim_2x.png"
    "${RES_DIR}/action_undo.png"
    "${RES_DIR}/action_undo_2x.png"
    "${RES_DIR}/action_zoom.png"
    "${RES_DIR}/action_zoom_2x.png"
    "${RES_DIR}/arrow_down.png"
    "${RES_DIR}/arrow_down_2x.png"
    "${RES_DIR}/arrow_up.png"
    "${RES_DIR}/arrow_up_2x.png"
    "${RES_DIR}/bankA.png"
    "${RES_DIR}/bankA_2x.png"
    "${RES_DIR}/bankB.png"
    "${RES_DIR}/bankB_2x.png"
    "${RES_DIR}/bankC.png"
    "${RES_DIR}/bankC_2x.png"
    "${RES_DIR}/bankD.png"
    "${RES_DIR}/bankD_2x.png"
    "${RES_DIR}/button.png"
    "${RES_DIR}/button_2x.png"
    "${RES_DIR}/button_loop.png"
    "${RES_DIR}/button_loop_2x.png"
    "${RES_DIR}/button_reverse.png"
    "${RES_DIR}/button_reverse_2x.png"
    "${RES_DIR}/edit.png"
    "${RES_DIR}/edit_2x.png"
    "${RES_DIR}/edit_edit_tab_background.png"
    "${RES_DIR}/edit_edit_tab_background_2x.png"
    "${RES_DIR}/edit_io_tab_background.png"
    "${RES_DIR}/edit_io_tab_background_2x.png"
    "${RES_DIR}/edit_sample_tab_background.png"
    "${RES_DIR}/edit_sample_tab_background_2x.png"
    "${RES_DIR}/knob_63frames.png"
    "${RES_DIR}/knob_63frames_2x.png"
    "${RES_DIR}/main_edit_tab_background.png"
    "${RES_DIR}/main_edit_tab_background_2x.png"
    "${RES_DIR}/main_play_tab_background.png"
    "${RES_DIR}/main_play_tab_background_2x.png"
    "${RES_DIR}/main_settings_tab_background.png"
    "${RES_DIR}/main_settings_tab_background_2x.png"
    "${RES_DIR}/main_view_background.png"
    "${RES_DIR}/main_view_background_2x.png"
    "${RES_DIR}/play.png"
    "${RES_DIR}/play_2x.png"
    "${RES_DIR}/quick_edit_mode.png"
    "${RES_DIR}/quick_edit_mode_2x.png"
    "${RES_DIR}/routing_mono_mono.png"
    "${RES_DIR}/routing_mono_mono_2x.png"
    "${RES_DIR}/routing_mono_stereo.png"
    "${RES_DIR}/routing_mono_stereo_2x.png"
    "${RES_DIR}/sample_input_In1.png"
    "${RES_DIR}/sample_input_In1_2x.png"
    "${RES_DIR}/sample_input_In2.png"
    "${RES_DIR}/sample_input_In2_2x.png"
    "${RES_DIR}/slice01.png"
    "${RES_DIR}/slice01_2x.png"
    "${RES_DIR}/slice16.png"
    "${RES_DIR}/slice16_2x.png"
    "${RES_DIR}/slice32.png"
    "${RES_DIR}/slice32_2x.png"
    "${RES_DIR}/slice48.png"
    "${RES_DIR}/slice48_2x.png"
    "${RES_DIR}/slice64.png"
    "${RES_DIR}/slice64_2x.png"
    "${RES_DIR}/slider_handle.png"
    "${RES_DIR}/slider_handle_2x.png"
    "${RES_DIR}/vu_on.png"
    "${RES_DIR}/vu_on_2x.png"
)

# Location of the test cases
set(TEST_DIR "${CMAKE_CURRENT_LIST_DIR}/test/cpp")

# List of test cases
set(test_case_sources
    "${TEST_DIR}/test-SampleBuffers.cpp"
    "${TEST_DIR}/test-SampleSlice.cpp"
    "${TEST_DIR}/test-SharedObjectMgr.cpp"
    "${TEST_DIR}/test-Slicer.cpp"
    )

# Finally invoke jamba_add_vst_plugin
jamba_add_vst_plugin(
    RELEASE_FILENAME         "SAM-SPL 64"
    ARCHIVE_ARCHITECTURE     "${JAMBA_ARCHIVE_ARCHITECTURE}"
    TARGETS_PREFIX           "jmb_"
    VST_SOURCES              "${vst_sources}" "${vst2_sources}"
    INCLUDE_DIRECTORIES      "${VERSION_DIR}"
    COMPILE_OPTIONS          "${PLUGIN_COMPILE_OPTIONS}"
    LINK_LIBRARIES           "sndfile" "r8brain-free-src" "miniaudio"
    UIDESC                   "${RES_DIR}/SampleSplitter.uidesc"
    RESOURCES                "${vst_resources}"
    TEST_CASE_SOURCES        "${test_case_sources}"
    TEST_INCLUDE_DIRECTORIES "${CPP_SOURCES}"
    TEST_LINK_LIBRARIES      "jamba"
)
