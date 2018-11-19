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

  // sampling related properties
  kSamplingInput = 2200,
  kSamplingMonitor = 2210,
  kSampling = 2220,

  //------------------------------------------------------------------------
  // Jmb Parameters
  //------------------------------------------------------------------------

  // the sample rate (communicated from RT to UI)
  kSampleRate = 3000,

  // The file sample loaded
  kFileSample = 3100,

  // The sample coming from sampling
  kSamplingSample = 3105,

  // keep track of settings for each slice
  kSlicesSettings = 3110,

  // The playing state
  kPlayingState = 3200,
};

}
}
}
