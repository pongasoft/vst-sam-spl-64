#pragma once

#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/vst/vsttypes.h>

namespace pongasoft::VST::SampleSplitter {

#ifndef NDEBUG
static const ::Steinberg::FUID SampleSplitterProcessorUID(0x96df4309, 0xbfcb4f89, 0x9c58202f, 0xda6ab1f1);
static const ::Steinberg::FUID SampleSplitterControllerUID(0xa4a65a05, 0xc59e4b5e, 0xbf80fbde, 0x26f174eb);
#else
static const ::Steinberg::FUID SampleSplitterProcessorUID(0x8dc749df, 0x8f164a07, 0x8fe50394, 0xb2632d61);
static const ::Steinberg::FUID SampleSplitterControllerUID(0x3eaace1e, 0x6ef14c74, 0x9cc54f8f, 0x33088aad);
#endif

//------------------------------------------------------------------------
// Parameters and Custom view ids
//------------------------------------------------------------------------
enum ESampleSplitterParamID : Steinberg::Vst::ParamID
{
  //------------------------------------------------------------------------
  // Vst Parameters
  //------------------------------------------------------------------------

  // Pads
  kPad1 = 2001,
  // ...
  // kPad16 2016
  
  kNumSlices = 2100,
  kPadBank = 2110,
  kSelectedSlice = 2120,
  kSelectedSliceViaMidi = 2121,
  kFollowMidiSelection = 2125,
  kPlayModeHold = 2130,
  kPolyphonic = 2140,
  kXFade = 2141,
  kInputRouting = 2142,
  kRootKey = 2145,
  kViewType = 2150,
  kEditingMode = 2155,

  // sampling related properties
  kSamplingInput = 2200,
  kSamplingInputGain = 2201,
  kSamplingMonitor = 2210,
  kSamplingDuration = 2211,
  kSampling = 2220,
  kSamplingLeftVuPPM = 2230,
  kSamplingRightVuPPM = 2231,
  kSamplingTrigger = 2240,


  // editing related properties
  kWEOffsetPercent = 2300,
  kWEZoomPercent = 2301,
  kWEShowZeroCrossing = 2302,
  kWEPlaySelection = 2303,
  kWEZoomToSelection = 2304,

  // saving related properties
  kExportSampleMajorFormat = 2400,
  kExportSampleMinorFormat = 2401,


  //------------------------------------------------------------------------
  // Jmb Parameters
  //------------------------------------------------------------------------

  // the sample rate (communicated from RT to UI)
  kSampleRate = 3000,

  // info about the host (communicated from RT to UI)
  kHostInfo = 3005,

  // The sample data
  kSampleData = 3100,
  kSampleDataMgr = 3101,

  // The sample buffers sent by the GUI to RT (message)
  kGUISampleMessage = 3102,

  // The sample buffers sent from the RT to the GUI after sampling (message)
  fRTSampleMessage = 3105,
  fSamplingState = 3106,

  // keep track of settings for each slice
  kSlicesSettings = 3110,
  kSlicesQuickEdit = 3120,

  // The playing state
  kPlayingState = 3200,

  // editing related properties
  kWESelectedSampleRange = 3300,

  //------------------------------------------------------------------------
  // Custom View Tag (not tied to params)
  //------------------------------------------------------------------------
  kNormalize0Action = 5000,
  kNormalize3Action = 5001,
  kNormalize6Action = 5002,
  kTrimAction = 5003,
  kCutAction = 5004,
  kCropAction = 5005,
  kResampleAction = 5006,
  kUndoAction = 5010,
  kRedoAction = 5011,
  kClearHistoryAction = 5012,
  kSampleAction = 5020,
};

}
