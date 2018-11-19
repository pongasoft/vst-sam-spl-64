#pragma once

#include "SampleSplitterCIDs.h"
#include "SampleBuffers.hpp"
#include "SampleSlice.h"
#include "Model.h"

#include <pongasoft/VST/Parameters.h>
#include <pongasoft/VST/RT/RTState.h>
#include <pongasoft/VST/GUI/GUIState.h>
#include <pongasoft/VST/GUI/Params/GUIParamSerializers.h>

#ifndef NDEBUG
#include <pongasoft/VST/Debug/ParamLine.h>
#include <pongasoft/VST/Debug/ParamTable.h>
#endif

#include <pluginterfaces/vst/ivstaudioprocessor.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

using namespace pongasoft::VST;
using namespace GUI::Params;

// keeping track of the version of the state being saved so that it can be upgraded more easily later
// should be > 0
constexpr uint16 PROCESSOR_STATE_VERSION = 1;
constexpr uint16 CONTROLLER_STATE_VERSION = 1;

constexpr const TChar *PAD_TITLES[NUM_PADS] = {
  STR16 ("Pad 1"),
  STR16 ("Pad 2"),
  STR16 ("Pad 3"),
  STR16 ("Pad 4"),
  STR16 ("Pad 5"),
  STR16 ("Pad 6"),
  STR16 ("Pad 7"),
  STR16 ("Pad 8"),
  STR16 ("Pad 9"),
  STR16 ("Pad 10"),
  STR16 ("Pad 11"),
  STR16 ("Pad 12"),
  STR16 ("Pad 13"),
  STR16 ("Pad 14"),
  STR16 ("Pad 15"),
  STR16 ("Pad 16"),
};

//------------------------------------------------------------------------
// SampleSplitterParameters
//------------------------------------------------------------------------
class SampleSplitterParameters : public Parameters
{
public:
  VstParam<int> fNumSlices;
  VstParam<int> fPadBank; // the bank/page representing 16 pads (4 banks of 16 pads => 64 pads)
  VstParam<int> fSelectedSlice; // keep track of which slice is selected (for settings editing purpose)
  VstParam<bool> fPlayModeHold; // hold (true) trigger (false)
  VstParam<bool> fPolyphonic; // if true => multiple pads can be "played" at the same time, if false => only 1
  VstParam<bool> fPads[NUM_PADS]; // 16 pads that are either on (momentary button pressed) or off

  VstParam<ESamplingInput> fSamplingInput; // which input to use for sampling (off, 1 or 2)
  VstParam<bool> fSamplingMonitor; // whether to copy input to output when sampling so it can be heard
  VstParam<bool> fSampling; // when true, RT will sample Stereo Input

  JmbParam<double> fSampleRate;
  JmbParam<PlayingState> fPlayingState;

  JmbParam<SampleBuffers32> fFileSample; // the sample coming from loading a file
  JmbParam<SampleBuffers32> fSamplingSample; // the sample coming from sampling
  JmbParam<SlicesSettings> fSlicesSettings; // maintain the settings per slice (forward/reverse, one shot/loop)

public:
  SampleSplitterParameters()
  {
    // which input to use for sampling (off, 1 or 2)
    fSamplingInput =
      vst<EnumParamConverter<ESamplingInput, ESamplingInput::kSamplingInput2>>(ESampleSplitterParamID::kSamplingInput, STR16("Sampling Input"),
                                                                               std::array<String, 3>{STR16("Off"), STR16("Input 1"), STR16("Input 2")})
        .defaultValue(kSamplingOff)
        .shortTitle(STR16("SamplingIn"))
        .add();

    // the number of slices the sample will be split into
    fNumSlices =
      vst<NumSlicesParamConverter>(ESampleSplitterParamID::kNumSlices, STR16("Num Slices"))
        .defaultValue(DEFAULT_NUM_SLICES)
        .shortTitle(STR16("Slices"))
        .add();

    // the bank/page representing 16 pads (4 banks of 16 pads => 64 pads)
    fPadBank =
      vst<DiscreteValueParamConverter<NUM_PAD_BANKS - 1>>(ESampleSplitterParamID::kPadBank, STR16("Page"),
                                                          STR16("Page %d"), 1)
        .defaultValue(0)
        .shortTitle(STR16("Page"))
        .add();

    // keep track of which slice is selected (for settings editing purpose)
    fSelectedSlice =
      vst<DiscreteValueParamConverter<NUM_SLICES -1>>(ESampleSplitterParamID::kSelectedSlice, STR16 ("Slice"),
                                                      1) // offset
        .defaultValue(0)
        .guiOwned()
        .add();

    // play mode => hold [true] (plays as long as held) trigger [false] (plays until the end of the slice or loop)
    fPlayModeHold =
      vst<BooleanParamConverter>(ESampleSplitterParamID::kPlayModeHold, STR16("Play Mode"),
                                 STR16("Trigger"), STR16("Hold")) // BooleanParamConverter args
        .defaultValue(true)
        .shortTitle(STR16("PlayMode"))
        .add();

    // if true => multiple pads can be "played" at the same time, if false => only 1
    fPolyphonic =
      vst<BooleanParamConverter>(ESampleSplitterParamID::kPolyphonic, STR16("Polyphony"),
                                 STR16("Mono"), STR16("Multi")) // BooleanParamConverter args
        .defaultValue(true)
        .shortTitle(STR16("Poly."))
        .add();


    // whether to copy input to output when sampling so it can be heard
    fSamplingMonitor =
      vst<BooleanParamConverter>(ESampleSplitterParamID::kSamplingMonitor, STR16("Monitor"))
        .defaultValue(false)
        .shortTitle(STR16("Monitor"))
        .add();

    // when true, RT will sample Stereo Input
    fSampling =
      vst<BooleanParamConverter>(ESampleSplitterParamID::kSampling, STR16("Sampling"))
        .defaultValue(false)
        .shortTitle(STR16("Sampling"))
        .transient()
        .add();

    // 16 pads that are either on (momentary button pressed) or off
    for(int i = 0; i < NUM_PADS; i++)
    {
      // pad 0
      fPads[i] =
        vst<BooleanParamConverter>(ESampleSplitterParamID::kPad1 + i, PAD_TITLES[i])
          .defaultValue(false)
          .flags(0)
          .shortTitle(PAD_TITLES[i])
          .transient()
          .add();
    }

    // the sample rate
    fSampleRate =
      jmb<DoubleParamSerializer>(ESampleSplitterParamID::kSampleRate, STR16 ("Sample Rate"))
        .defaultValue(0)
        .rtOwned()
        .transient()
        .shared()
        .add();

    // playing state
    fPlayingState =
      jmb<PlayingStateParamSerializer>(ESampleSplitterParamID::kPlayingState, STR16 ("Playing State"))
        .rtOwned()
        .transient()
        .shared()
        .add();

    // the file sample
    fFileSample =
      jmb<SampleBuffersSerializer32>(ESampleSplitterParamID::kFileSample, STR16 ("File Sample"))
        .guiOwned()
        .shared()
        .add();

    // the sampling sample
    fSamplingSample =
      jmb<SampleBuffersSerializer32>(ESampleSplitterParamID::kSamplingSample, STR16 ("Sampling Sample"))
        .rtOwned()
        .shared()
        .transient()
        .add();

    // the settings per slice (forward/reverse, one shot/loop)
    fSlicesSettings =
      jmb<SlicesSettingsParamSerializer>(ESampleSplitterParamID::kSlicesSettings, STR16 ("Slices Settings"))
        .guiOwned()
        .shared()
        .add();

    // RT save state order
    setRTSaveStateOrder(PROCESSOR_STATE_VERSION,
                        fNumSlices,
                        fPadBank,
                        fPlayModeHold,
                        fPolyphonic,
                        fSamplingInput,
                        fSamplingMonitor);

    // GUI save state order
    setGUISaveStateOrder(CONTROLLER_STATE_VERSION,
                         fFileSample,
                         fSelectedSlice,
                         fSlicesSettings);
  }
};

using namespace RT;

//------------------------------------------------------------------------
// SampleSplitterRTState
//------------------------------------------------------------------------
class SampleSplitterRTState : public RTState
{
public:
  RTVstParam<int> fNumSlices;
  RTVstParam<int> fPadBank;
  RTVstParam<bool> fPlayModeHold;
  RTVstParam<bool> fPolyphonic;
  RTVstParam<bool> *fPads[NUM_PADS];

  RTVstParam<ESamplingInput> fSamplingInput;
  RTVstParam<bool> fSamplingMonitor;
  RTVstParam<bool> fSampling;

  RTJmbOutParam<SampleRate> fSampleRate;
  RTJmbOutParam<PlayingState> fPlayingState;

  // When a new sample is loaded, the UI will send it to the RT
  RTJmbInParam<SampleBuffers32> fFileSampleMessage;

  // When sampling is complete, the RT will send it to the UI
  RTJmbOutParam<SampleBuffers32> fSamplingSample;

  // UI maintains the slices settings (RT cannot handle this type)
  RTJmbInParam<SlicesSettings> fSlicesSettings;

  SampleBuffers32 fFileSample;
  SampleSlice fSampleSlices[NUM_SLICES];

public:
  explicit SampleSplitterRTState(SampleSplitterParameters const &iParams) :
    RTState(iParams),
    fNumSlices{add(iParams.fNumSlices)},
    fPadBank{add(iParams.fPadBank)},
    fPlayModeHold{add(iParams.fPlayModeHold)},
    fPolyphonic{add(iParams.fPolyphonic)},
    fPads{nullptr},
    fSamplingInput{add(iParams.fSamplingInput)},
    fSamplingMonitor{add(iParams.fSamplingMonitor)},
    fSampling{add(iParams.fSampling)},
    fSampleRate{addJmbOut(iParams.fSampleRate)},
    fPlayingState{addJmbOut(iParams.fPlayingState)},
    fFileSampleMessage{addJmbIn(iParams.fFileSample)},
    fSamplingSample{addJmbOut(iParams.fSamplingSample)},
    fSlicesSettings{addJmbIn(iParams.fSlicesSettings)},
    fFileSample{0},
    fSampleSlices{}
  {
    for(int i = 0; i < NUM_PADS; i++)
    {
      fPads[i] = new RTVstParam<bool>(add(iParams.fPads[i]));
    }
  }

  ~SampleSplitterRTState() override
  {
    for(auto &fPad : fPads)
    {
      delete fPad;
    }
  }

//------------------------------------------------------------------------
// Debug read/write RT state
//------------------------------------------------------------------------
#ifndef NDEBUG
protected:
  // afterReadNewState
  void afterReadNewState(NormalizedState const *iState) override
  {
    // swap the commented line to display either on a line or in a table
    DLOG_F(INFO, "RTState::read - %s", Debug::ParamLine::from(this, true).toString(*iState).c_str());
    //Debug::ParamTable::from(this, true).showCellSeparation().print(*iState, "RTState::read ---> ");
  }

  // beforeWriteNewState
  void beforeWriteNewState(NormalizedState const *iState) override
  {
    // swap the commented line to display either on a line or in a table
    DLOG_F(INFO, "RTState::write - %s", Debug::ParamLine::from(this, true).toString(*iState).c_str());
    //Debug::ParamTable::from(this, true).showCellSeparation().print(*iState, "RTState::write ---> ");
  }
#endif
};

using namespace GUI;

//------------------------------------------------------------------------
// SampleSplitterGUIState
//------------------------------------------------------------------------
class SampleSplitterGUIState : public GUIPluginState<SampleSplitterParameters>
{
public:
  GUIJmbParam<SampleRate> fSampleRate;
  GUIJmbParam<PlayingState> fPlayingState;
  GUIJmbParam<SampleBuffers32> fFileSample;
  GUIJmbParam<SampleBuffers32> fSamplingSample;
  GUIJmbParam<SlicesSettings> fSlicesSettings;

public:
  explicit SampleSplitterGUIState(SampleSplitterParameters const &iParams) :
    GUIPluginState(iParams),
    fSampleRate{add(iParams.fSampleRate)},
    fPlayingState{add(iParams.fPlayingState)},
    fFileSample{add(iParams.fFileSample)},
    fSamplingSample{add(iParams.fSamplingSample)},
    fSlicesSettings{add(iParams.fSlicesSettings)}
  {};

  // broadcastSample
  tresult broadcastSample();

protected:
  // readGUIState
  tresult readGUIState(IBStreamer &iStreamer) override;

#ifndef NDEBUG
  // writeGUIState
  tresult writeGUIState(IBStreamer &oStreamer) const override
  {
    tresult res = GUIState::writeGUIState(oStreamer);
    if(res == kResultOk)
    {
      // swap the commented line to display either on a line or in a table
      DLOG_F(INFO, "GUIState::write - %s", Debug::ParamLine::from(this, true).toString().c_str());
      //Debug::ParamTable::from(this, true).showCellSeparation().print("GUIState::write ---> ");
    }
    return res;
  }
#endif

};

}
}
}
