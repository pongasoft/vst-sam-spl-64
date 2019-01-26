#pragma once

#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/vst/vsttypes.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

static const ::Steinberg::FUID SampleSplitterProcessorUID(0x8dc749df, 0x8f164a07, 0x8fe50394, 0xb2632d61);
static const ::Steinberg::FUID SampleSplitterControllerUID(0x3eaace1e, 0x6ef14c74, 0x9cc54f8f, 0x33088aad);

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
  kPlayModeHold = 2130,
  kPolyphonic = 2140,
  kViewType = 2150,

  // sampling related properties
  kSamplingInput = 2200,
  kSamplingMonitor = 2210,
  kSampling = 2220,

  // editing related properties
  kWEOffsetPercent = 2300,
  kWEZoomPercent = 2301,
  kWEShowZeroCrossing = 2302,
  kWEPlaySelection = 2303,

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

  // The sample buffers sent by the GUI to RT (message)
  kGUISampleMessage = 3102,

  // The sample buffers sent from the RT to the GUI after sampling (message)
  fRTSampleMessage = 3105,
  fSamplingState = 3106,

  // keep track of settings for each slice
  kSlicesSettings = 3110,

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
  kSampleAction = 5020,
};

}
}
}
