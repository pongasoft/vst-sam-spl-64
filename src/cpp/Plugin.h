#pragma once

#include "SampleSplitterCIDs.h"
#include "SampleBuffers.hpp"
#include "SampleSlice.h"
#include "SampleData.h"
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
  VstParam<EViewType> fViewType; // which view to show (main or edit sample)

  VstParam<int> fNumSlices;
  VstParam<int> fPadBank; // the bank/page representing 16 pads (4 banks of 16 pads => 64 pads)
  VstParam<int> fSelectedSlice; // keep track of which slice is selected (for settings editing purpose)
  VstParam<bool> fPlayModeHold; // hold (true) trigger (false)
  VstParam<bool> fPolyphonic; // if true => multiple pads can be "played" at the same time, if false => only 1
  VstParam<bool> fPads[NUM_PADS]; // 16 pads that are either on (momentary button pressed) or off

  ///// sampling
  VstParam<ESamplingInput> fSamplingInput; // which input to use for sampling (off, 1 or 2)
  VstParam<bool> fSamplingMonitor; // whether to copy input to output when sampling so it can be heard
  VstParam<bool> fSampling; // when true, RT will sample Stereo Input

  ///// editing (WE = WaveformEdit)
  RawVstParam fWEOffsetPercent;
  RawVstParam fWEZoomPercent;
  VstParam<bool> fWEShowZeroCrossing;
  JmbParam<SampleRange> fWESelectedSampleRange;
  VstParam<bool> fWEPlaySelection;

  VstParam<SampleStorage::ESampleMajorFormat> fExportSampleMajorFormat;
  VstParam<SampleStorage::ESampleMinorFormat> fExportSampleMinorFormat;

  JmbParam<double> fSampleRate;
  JmbParam<HostInfo> fHostInfoMessage;
  JmbParam<PlayingState> fPlayingState;

  JmbParam<SampleData> fSampleData; // the sample data itself
  JmbParam<SampleBuffers32> fGUISampleMessage; // the sample (sent from the GUI to RT)
  JmbParam<SampleBuffers32> fRTSampleMessage; // the sample (sent from RT to GUI)
  JmbParam<SamplingState> fSamplingState; // during sampling, RT will provide updates
  JmbParam<SlicesSettings> fSlicesSettings; // maintain the settings per slice (forward/reverse, one shot/loop)

public:
  SampleSplitterParameters();
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
  RTJmbOutParam<HostInfo> fHostInfoMessage;
  RTJmbOutParam<PlayingState> fPlayingState;

  // When a new sample is loaded, the UI will send it to the RT
  RTJmbInParam<SampleBuffers32> fGUISampleMessage;

  // When sampling is complete, the RT will send it to the UI
  RTJmbOutParam<SampleBuffers32> fRTSampleMessage;
  RTJmbOutParam<SamplingState> fSamplingState;

  // UI maintains the slices settings (RT cannot handle this type)
  RTJmbInParam<SlicesSettings> fSlicesSettings;

  // Selected range
  RTJmbInParam<SampleRange> fWESelectedSampleRange;
  RTVstParam<bool> fWEPlaySelection;

  SampleBuffers32 fSampleBuffers;
  SampleSlice fSampleSlices[NUM_SLICES];
  SampleSlice fWESelectionSlice{};
  HostInfo fHostInfo;

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
    fHostInfoMessage{addJmbOut(iParams.fHostInfoMessage)},
    fPlayingState{addJmbOut(iParams.fPlayingState)},
    fGUISampleMessage{addJmbIn(iParams.fGUISampleMessage)},
    fRTSampleMessage{addJmbOut(iParams.fRTSampleMessage)},
    fSamplingState{addJmbOut(iParams.fSamplingState)},
    fSlicesSettings{addJmbIn(iParams.fSlicesSettings)},
    fWESelectedSampleRange{addJmbIn(iParams.fWESelectedSampleRange)},
    fWEPlaySelection{add(iParams.fWEPlaySelection)},
    fSampleBuffers{0},
    fSampleSlices{},
    fHostInfo{}
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
  GUIJmbParam<HostInfo> fHostInfo;
  GUIJmbParam<PlayingState> fPlayingState;
  GUIJmbParam<SampleData> fSampleData;
  GUIJmbParam<SampleBuffers32> fRTSampleMessage;
  GUIJmbParam<SamplingState> fSamplingState;
  GUIJmbParam<SlicesSettings> fSlicesSettings;
  GUIJmbParam<SampleRange> fWESelectedSampleRange;

public:
  explicit SampleSplitterGUIState(SampleSplitterParameters const &iParams) :
    GUIPluginState(iParams),
    fSampleRate{add(iParams.fSampleRate)},
    fHostInfo{add(iParams.fHostInfoMessage)},
    fPlayingState{add(iParams.fPlayingState)},
    fSampleData{add(iParams.fSampleData)},
    fRTSampleMessage{add(iParams.fRTSampleMessage)},
    fSamplingState{add(iParams.fSamplingState)},
    fSlicesSettings{add(iParams.fSlicesSettings)},
    fWESelectedSampleRange{add(iParams.fWESelectedSampleRange)}
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
