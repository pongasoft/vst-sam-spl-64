cmake_minimum_required(VERSION 3.12)

# build VST 2?
option(JAMBA_ENABLE_VST2 "Use VST2" ON)

# build Audio Unit?
option(JAMBA_ENABLE_AUDIO_UNIT "Enable Audio Unit" ON)

set(PLUGIN_MAJOR_VERSION 1)
set(PLUGIN_MINOR_VERSION 2)
set(PLUGIN_PATCH_VERSION 0)
set(PLUGIN_VERSION "${PLUGIN_MAJOR_VERSION}.${PLUGIN_MINOR_VERSION}.${PLUGIN_PATCH_VERSION}")

project(SamSpl64)

# Optional: To use local libsndfile (no download), provide path to where it is located
unset(LIBSNDFILE_ROOT_DIR)
#set(LIBSNDFILE_ROOT_DIR ../../erikd/libsndfile)

# Optional: To use local r8brain-free-src (no download), provide path to where it is located
unset(R8BRAIN-FREE-SRC_ROOT_DIR)
#set(R8BRAIN-FREE-SRC_ROOT_DIR ../../pongasoft/r8brain-free-src)

# Optional: To use local jamba install, uncomment the following line (no download)
unset(JAMBA_ROOT_DIR)
set(JAMBA_ROOT_DIR "../../pongasoft/jamba")

# Optional: To use local googletest install, uncomment the following line (no download) and modify the path accordingly
unset(GOOGLETEST_ROOT_DIR)
#set(GOOGLETEST_ROOT_DIR ../../google/googletest)

# including those before VST to avoid warning messages
include(libsndfile.cmake)
include(r8brain-free-src.cmake)

include(jamba.cmake)

set(CPP_SOURCES src/cpp)

configure_file(${CPP_SOURCES}/version.h.in ${CMAKE_BINARY_DIR}/generated/version.h)
include_directories(${CMAKE_BINARY_DIR}/generated/)

if (JAMBA_ENABLE_VST2)
  set(vst2_sources
      ${CPP_SOURCES}/SampleSplitter_VST2.cpp
      )
endif ()

set(vst_sources
    ${CPP_SOURCES}/SampleSplitter_VST3.cpp

    ${CPP_SOURCES}/FilePath.h
    ${CPP_SOURCES}/FilePath.cpp
    ${CPP_SOURCES}/Model.h
    ${CPP_SOURCES}/Plugin.h
    ${CPP_SOURCES}/Plugin.cpp
    ${CPP_SOURCES}/SampleBuffers.h
    ${CPP_SOURCES}/SampleBuffers.hpp
    ${CPP_SOURCES}/SampleBuffers.cpp
    ${CPP_SOURCES}/SampleData.h
    ${CPP_SOURCES}/SampleData.cpp
    ${CPP_SOURCES}/SampleDataMgr.h
    ${CPP_SOURCES}/SampleDataMgr.cpp
    ${CPP_SOURCES}/SampleFile.h
    ${CPP_SOURCES}/SampleFile.cpp
    ${CPP_SOURCES}/Sampler.h
    ${CPP_SOURCES}/Sampler.hpp
    ${CPP_SOURCES}/SampleSlice.hpp
    ${CPP_SOURCES}/SampleSlices.hpp
    ${CPP_SOURCES}/SampleSplitterCIDs.h
    ${CPP_SOURCES}/SampleStorage.h
    ${CPP_SOURCES}/Slicer.hpp

    ${CPP_SOURCES}/RT/SampleSplitterProcessor.h
    ${CPP_SOURCES}/RT/SampleSplitterProcessor.cpp

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
    ${CPP_SOURCES}/GUI/SampleEditScrollbarView.h
    ${CPP_SOURCES}/GUI/SampleEditScrollbarView.cpp
    ${CPP_SOURCES}/GUI/SampleEditController.h
    ${CPP_SOURCES}/GUI/SampleEditController.cpp
    ${CPP_SOURCES}/GUI/SampleInfoView.h
    ${CPP_SOURCES}/GUI/SampleInfoView.cpp
    ${CPP_SOURCES}/GUI/SampleLoaderView.h
    ${CPP_SOURCES}/GUI/SampleLoaderView.cpp
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

# VST2 is only defined for macOS and Windows
if (MAC OR WIN)
  set(vst_sources ${vst_sources} ${vst2_sources})
endif ()

#-------------------------------------------------------------------------------
# Compiler options
#-------------------------------------------------------------------------------
if(WIN32)
  message(STATUS "Adding compiler options")
  # suppress warning about std::copy using unsafe iterators
  add_compile_options("/D_SCL_SECURE_NO_WARNINGS")
endif()

set(target pongasoft_sam_spl_64)
jamba_add_vst3plugin(${target} "${vst_sources}")
target_include_directories(${target} PUBLIC ${VSTGUI_ROOT}/vstgui4)
target_link_libraries(${target} PRIVATE base sdk vstgui_support sndfile r8brain-free-src jamba)

###################################################
# Resources - add your resources here...
###################################################
jamba_add_vst3_resource(${target} DATA "SampleSplitter.uidesc")
jamba_add_vst3_resource(${target} PNG "action_clear.png")
jamba_add_vst3_resource(${target} PNG "action_clear_2x.png")
jamba_add_vst3_resource(${target} PNG "action_crop.png")
jamba_add_vst3_resource(${target} PNG "action_crop_2x.png")
jamba_add_vst3_resource(${target} PNG "action_cut.png")
jamba_add_vst3_resource(${target} PNG "action_cut_2x.png")
jamba_add_vst3_resource(${target} PNG "action_export.png")
jamba_add_vst3_resource(${target} PNG "action_export_2x.png")
jamba_add_vst3_resource(${target} PNG "action_load.png")
jamba_add_vst3_resource(${target} PNG "action_load_2x.png")
jamba_add_vst3_resource(${target} PNG "action_match.png")
jamba_add_vst3_resource(${target} PNG "action_match_2x.png")
jamba_add_vst3_resource(${target} PNG "action_norm_0dB.png")
jamba_add_vst3_resource(${target} PNG "action_norm_0dB_2x.png")
jamba_add_vst3_resource(${target} PNG "action_norm_3dB.png")
jamba_add_vst3_resource(${target} PNG "action_norm_3dB_2x.png")
jamba_add_vst3_resource(${target} PNG "action_norm_6dB.png")
jamba_add_vst3_resource(${target} PNG "action_norm_6dB_2x.png")
jamba_add_vst3_resource(${target} PNG "action_redo.png")
jamba_add_vst3_resource(${target} PNG "action_redo_2x.png")
jamba_add_vst3_resource(${target} PNG "action_reset.png")
jamba_add_vst3_resource(${target} PNG "action_reset_2x.png")
jamba_add_vst3_resource(${target} PNG "action_sample.png")
jamba_add_vst3_resource(${target} PNG "action_sample_2x.png")
jamba_add_vst3_resource(${target} PNG "action_switch_edit_edit_tab.png")
jamba_add_vst3_resource(${target} PNG "action_switch_edit_edit_tab_2x.png")
jamba_add_vst3_resource(${target} PNG "action_switch_edit_io_tab.png")
jamba_add_vst3_resource(${target} PNG "action_switch_edit_io_tab_2x.png")
jamba_add_vst3_resource(${target} PNG "action_switch_edit_sample_tab.png")
jamba_add_vst3_resource(${target} PNG "action_switch_edit_sample_tab_2x.png")
jamba_add_vst3_resource(${target} PNG "action_toggle_slice_loop_all.png")
jamba_add_vst3_resource(${target} PNG "action_toggle_slice_loop_all_2x.png")
jamba_add_vst3_resource(${target} PNG "action_trim.png")
jamba_add_vst3_resource(${target} PNG "action_trim_2x.png")
jamba_add_vst3_resource(${target} PNG "action_undo.png")
jamba_add_vst3_resource(${target} PNG "action_undo_2x.png")
jamba_add_vst3_resource(${target} PNG "action_zoom.png")
jamba_add_vst3_resource(${target} PNG "action_zoom_2x.png")
jamba_add_vst3_resource(${target} PNG "arrow_down.png")
jamba_add_vst3_resource(${target} PNG "arrow_down_2x.png")
jamba_add_vst3_resource(${target} PNG "arrow_up.png")
jamba_add_vst3_resource(${target} PNG "arrow_up_2x.png")
jamba_add_vst3_resource(${target} PNG "bankA.png")
jamba_add_vst3_resource(${target} PNG "bankA_2x.png")
jamba_add_vst3_resource(${target} PNG "bankB.png")
jamba_add_vst3_resource(${target} PNG "bankB_2x.png")
jamba_add_vst3_resource(${target} PNG "bankC.png")
jamba_add_vst3_resource(${target} PNG "bankC_2x.png")
jamba_add_vst3_resource(${target} PNG "bankD.png")
jamba_add_vst3_resource(${target} PNG "bankD_2x.png")
jamba_add_vst3_resource(${target} PNG "button.png")
jamba_add_vst3_resource(${target} PNG "button_2x.png")
jamba_add_vst3_resource(${target} PNG "button_loop.png")
jamba_add_vst3_resource(${target} PNG "button_loop_2x.png")
jamba_add_vst3_resource(${target} PNG "button_reverse.png")
jamba_add_vst3_resource(${target} PNG "button_reverse_2x.png")
jamba_add_vst3_resource(${target} PNG "edit.png")
jamba_add_vst3_resource(${target} PNG "edit_2x.png")
jamba_add_vst3_resource(${target} PNG "edit_edit_tab_background.png")
jamba_add_vst3_resource(${target} PNG "edit_edit_tab_background_2x.png")
jamba_add_vst3_resource(${target} PNG "edit_io_tab_background.png")
jamba_add_vst3_resource(${target} PNG "edit_io_tab_background_2x.png")
jamba_add_vst3_resource(${target} PNG "edit_sample_tab_background.png")
jamba_add_vst3_resource(${target} PNG "edit_sample_tab_background_2x.png")
jamba_add_vst3_resource(${target} PNG "main_edit_tab_background.png")
jamba_add_vst3_resource(${target} PNG "main_edit_tab_background_2x.png")
jamba_add_vst3_resource(${target} PNG "main_play_tab_background.png")
jamba_add_vst3_resource(${target} PNG "main_play_tab_background_2x.png")
jamba_add_vst3_resource(${target} PNG "main_settings_tab_background.png")
jamba_add_vst3_resource(${target} PNG "main_settings_tab_background_2x.png")
jamba_add_vst3_resource(${target} PNG "main_view_background.png")
jamba_add_vst3_resource(${target} PNG "main_view_background_2x.png")
jamba_add_vst3_resource(${target} PNG "play.png")
jamba_add_vst3_resource(${target} PNG "play_2x.png")
jamba_add_vst3_resource(${target} PNG "quick_edit_mode.png")
jamba_add_vst3_resource(${target} PNG "quick_edit_mode_2x.png")
jamba_add_vst3_resource(${target} PNG "routing_mono_mono.png")
jamba_add_vst3_resource(${target} PNG "routing_mono_mono_2x.png")
jamba_add_vst3_resource(${target} PNG "routing_mono_stereo.png")
jamba_add_vst3_resource(${target} PNG "routing_mono_stereo_2x.png")
jamba_add_vst3_resource(${target} PNG "sample_input_In1.png")
jamba_add_vst3_resource(${target} PNG "sample_input_In1_2x.png")
jamba_add_vst3_resource(${target} PNG "sample_input_In2.png")
jamba_add_vst3_resource(${target} PNG "sample_input_In2_2x.png")
jamba_add_vst3_resource(${target} PNG "slice01.png")
jamba_add_vst3_resource(${target} PNG "slice01_2x.png")
jamba_add_vst3_resource(${target} PNG "slice02.png")
jamba_add_vst3_resource(${target} PNG "slice02_2x.png")
jamba_add_vst3_resource(${target} PNG "slice04.png")
jamba_add_vst3_resource(${target} PNG "slice04_2x.png")
jamba_add_vst3_resource(${target} PNG "slice08.png")
jamba_add_vst3_resource(${target} PNG "slice08_2x.png")
jamba_add_vst3_resource(${target} PNG "slice16.png")
jamba_add_vst3_resource(${target} PNG "slice16_2x.png")
jamba_add_vst3_resource(${target} PNG "slice32.png")
jamba_add_vst3_resource(${target} PNG "slice32_2x.png")
jamba_add_vst3_resource(${target} PNG "slice48.png")
jamba_add_vst3_resource(${target} PNG "slice48_2x.png")
jamba_add_vst3_resource(${target} PNG "slice64.png")
jamba_add_vst3_resource(${target} PNG "slice64_2x.png")
jamba_add_vst3_resource(${target} PNG "slider_handle.png")
jamba_add_vst3_resource(${target} PNG "slider_handle_2x.png")
jamba_add_vst3_resource(${target} PNG "vu_on.png")
jamba_add_vst3_resource(${target} PNG "vu_on_2x.png")

# this must be called after you are done calling jamba_add_vst3_resource for all resources
jamba_gen_vst3_resources(${target} "SampleSplitter")

###################################################
# Create archive (.tgz)
###################################################
jamba_create_archive(${target} "SAM-SPL 64")

####################################################
## Testing
####################################################
file(GLOB_RECURSE TEST_SRC_FILES RELATIVE ${PROJECT_SOURCE_DIR} test/cpp/*cpp)
set(test_sources ${test_sources}
    ${LOGURU_IMPL}
    )
set(test_libs base sdk vstgui_support jamba)

jamba_add_test(${target}_test "${TEST_SRC_FILES}" "${test_sources}" "${test_libs}")

###################################################
# Dev help
###################################################
jamba_dev_scripts(${target})
