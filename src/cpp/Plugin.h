#pragma once

#include "SampleSplitterCIDs.h"
#include "SampleBuffers.hpp"
#include "SharedSampleBuffersMgr.h"
#include "SampleSlices.hpp"
#include "GUI/CurrentSample.h"
#include "GUI/SampleFile.h"
#include "GUI/UndoHistory.h"
#include "Model.h"
#include <optional>

#include <pongasoft/VST/Parameters.h>
#include <pongasoft/VST/RT/RTState.h>
#include <pongasoft/VST/GUI/GUIState.h>
#include <pongasoft/VST/GUI/Params/GUIParamSerializers.h>

#ifndef NDEBUG
#include <pongasoft/VST/Debug/ParamLine.h>
#include <pongasoft/VST/Debug/ParamTable.h>
#endif

#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include "FilePath.h"

namespace pongasoft::VST::SampleSplitter {

using namespace pongasoft::VST;
using namespace pongasoft::VST::GUI::Params;

// keeping track of the version of the state being saved so that it can be upgraded more easily later
// should be > 0
constexpr uint16 kProcessorStateLatest = 2;
constexpr uint16 kControllerStateLatest = 1;

// Deprecated versions
constexpr uint16 kProcessorStateV1 = 1;

using error_message_t = std::optional<std::string>;

//------------------------------------------------------------------------
// SampleSplitterParameters
//------------------------------------------------------------------------
class SampleSplitterParameters : public Parameters
{
public:
  VstParam<NumSlice> fNumSlices;
  VstParam<EViewType> fViewType; // which view to show (main/edit)
  VstParam<EEditingMode> fEditingMode; // which subtab to show (edit/sample)

  VstParam<int> fPadBank; // the bank/page representing 16 pads (4 banks of 16 pads => 64 pads)
  VstParam<int> fSelectedSlice; // keep track of which slice is selected (for settings editing purpose)
  VstParam<int> fSelectedSliceViaMidi; // keep track of which slice is selected via Midi
  VstParam<bool> fFollowMidiSelection; // whether midi input changes the selected slice
  VstParam<bool> fPlayModeHold; // hold (true) trigger (false)
  VstParam<bool> fPolyphonic; // if true => multiple pads can be "played" at the same time, if false => only 1
  VstParam<bool> fXFade; // whether to cross fade (start/stop and looping can create clicks and pops)
  VstParam<EInputRouting> fInputRouting; // how to handle input routing (mono->mono or mono->stereo)
  VstParam<RootKey> fRootKey; // the root key to use (first pad)
  VstParam<bool> fPads[NUM_PADS]; // 16 pads that are either on (momentary button pressed) or off
  JmbParam<bool> fSlicesQuickEdit; // whether we are editing all slices at once

  ///// sampling
  VstParam<int> fSamplingDurationInBars; // how long to sample for (in multiple of bars)
  VstParam<ESamplingInput> fSamplingInput; // which input to use for sampling (off, 1 or 2)
  VstParam<Gain> fSamplingInputGain; // gain for sampling input
  VstParam<bool> fSamplingMonitor; // whether to copy input to output when sampling so it can be heard
  VstParam<bool> fSampling; // when true, RT will sample Stereo Input
  RawVstParam fSamplingLeftVuPPM; // VU PPM (left channel) for the selected input for sampling
  RawVstParam fSamplingRightVuPPM; // VU PPM (right channel) for the selected input for sampling
  VstParam<ESamplingTrigger> fSamplingTrigger; // what triggers sampling

  ///// editing (WE = WaveformEdit)
  RawVstParam fWEOffsetPercent;
  RawVstParam fWEZoomPercent;
  VstParam<bool> fWEShowZeroCrossing;
  JmbParam<SampleRange> fWESelectedSampleRange;
  VstParam<bool> fWEPlaySelection;
  VstParam<bool> fWEZoomToSelection;

  VstParam<GUI::SampleFile::ESampleMajorFormat> fExportSampleMajorFormat;
  VstParam<GUI::SampleFile::ESampleMinorFormat> fExportSampleMinorFormat;

  JmbParam<double> fSampleRate;
  JmbParam<HostInfo> fHostInfoMessage;
  JmbParam<PlayingState> fPlayingState;

  JmbParam<GUI::CurrentSample> fCurrentSample; // the current sample in the GUI
  JmbParam<GUI::SampleFile> fSampleFile; // the sample file
  JmbParam<GUI::UndoHistory> fUndoHistory; // the undo history
  JmbParam<SharedSampleBuffersVersion> fGUINewSampleMessage; // when a sample is loaded in the GUI, it notifies RT about it
  JmbParam<SharedSampleBuffersVersion> fRTNewSampleMessage; // after sampling in RT, it notifies GUI about it
  JmbParam<SamplingState> fSamplingState; // during sampling, RT will provide updates
  JmbParam<SlicesSettings> fSlicesSettings; // maintain the settings per slice (forward/reverse, one shot/loop)
  JmbParam<UTF8Path> fLargeFilePath;
  JmbParam<error_message_t> fErrorMessage;
  JmbParam<SharedSampleBuffersMgr32 *> fSharedSampleBuffersMgrPtr; // the shared mgr

  JmbParam<std::string> fPluginVersion;

  // Deprecated parameters
  VstParam<int> __deprecated_fNumSlices;

public:
  SampleSplitterParameters();

protected:
  tresult handleRTStateUpgrade(NormalizedState const &iDeprecatedState, NormalizedState &oNewState) const override;
};

using namespace RT;

//------------------------------------------------------------------------
// SampleSplitterRTState
//------------------------------------------------------------------------
class SampleSplitterRTState : public RTState
{
public:
  RTVstParam<NumSlice> fNumSlices;
  RTVstParam<int> fPadBank;
  RTVstParam<int> fSelectedSliceViaMidi;
  RTVstParam<bool> fFollowMidiSelection;
  RTVstParam<bool> fPlayModeHold;
  RTVstParam<bool> fPolyphonic;
  RTVstParam<bool> fXFade;
  RTVstParam<EInputRouting> fInputRouting;
  RTVstParam<RootKey> fRootKey;
  RTVstParam<bool> *fPads[NUM_PADS];

  RTVstParam<int> fSamplingDurationInBars;
  RTVstParam<ESamplingInput> fSamplingInput;
  RTVstParam<Gain> fSamplingInputGain;
  RTVstParam<bool> fSamplingMonitor;
  RTVstParam<bool> fSampling;
  RTRawVstParam fSamplingLeftVuPPM;
  RTRawVstParam fSamplingRightVuPPM;
  RTVstParam<ESamplingTrigger> fSamplingTrigger;

  RTJmbOutParam<SampleRate> fSampleRate;
  RTJmbOutParam<HostInfo> fHostInfoMessage;
  RTJmbOutParam<PlayingState> fPlayingState;

  // When a new sample is loaded, the UI will let the RT know
  RTJmbInParam<SharedSampleBuffersVersion> fGUINewSampleMessage;

  // When sampling is complete, the RT will let the UI know
  RTJmbOutParam<SharedSampleBuffersVersion> fRTNewSampleMessage;

  RTJmbOutParam<SamplingState> fSamplingState;
  RTJmbOutParam<SharedSampleBuffersMgr32 *> fSharedSampleBuffersMgrPtr; // the shared mgr

  // UI maintains the slices settings (RT cannot handle this type)
  RTJmbInParam<SlicesSettings> fSlicesSettings;

  // Selected range
  RTJmbInParam<SampleRange> fWESelectedSampleRange;
  RTVstParam<bool> fWEPlaySelection;

  SampleSlices<NUM_SLICES> fSampleSlices;

  SharedSampleBuffersMgr32 fSharedSampleBuffersMgr{};

//  SampleSlice fWESelectionSlice{};
  HostInfo fHostInfo;

public:
  explicit SampleSplitterRTState(SampleSplitterParameters const &iParams) :
    RTState(iParams),
    fNumSlices{add(iParams.fNumSlices)},
    fPadBank{add(iParams.fPadBank)},
    fSelectedSliceViaMidi{add(iParams.fSelectedSliceViaMidi)},
    fFollowMidiSelection{add(iParams.fFollowMidiSelection)},
    fPlayModeHold{add(iParams.fPlayModeHold)},
    fPolyphonic{add(iParams.fPolyphonic)},
    fXFade{add(iParams.fXFade)},
    fInputRouting{add(iParams.fInputRouting)},
    fRootKey{add(iParams.fRootKey)},
    fPads{nullptr},
    fSamplingDurationInBars{add(iParams.fSamplingDurationInBars)},
    fSamplingInput{add(iParams.fSamplingInput)},
    fSamplingInputGain{add(iParams.fSamplingInputGain)},
    fSamplingMonitor{add(iParams.fSamplingMonitor)},
    fSampling{add(iParams.fSampling)},
    fSamplingLeftVuPPM{add(iParams.fSamplingLeftVuPPM)},
    fSamplingRightVuPPM{add(iParams.fSamplingRightVuPPM)},
    fSamplingTrigger{add(iParams.fSamplingTrigger)},
    fSampleRate{addJmbOut(iParams.fSampleRate)},
    fHostInfoMessage{addJmbOut(iParams.fHostInfoMessage)},
    fPlayingState{addJmbOut(iParams.fPlayingState)},
    fGUINewSampleMessage{addJmbIn(iParams.fGUINewSampleMessage)},
    fRTNewSampleMessage{addJmbOut(iParams.fRTNewSampleMessage)},
    fSharedSampleBuffersMgrPtr{addJmbOut(iParams.fSharedSampleBuffersMgrPtr)},
    fSamplingState{addJmbOut(iParams.fSamplingState)},
    fSlicesSettings{addJmbIn(iParams.fSlicesSettings)},
    fWESelectedSampleRange{addJmbIn(iParams.fWESelectedSampleRange)},
    fWEPlaySelection{add(iParams.fWEPlaySelection)},
    fSampleSlices{},
    fHostInfo{}
  {
    for(int i = 0; i < NUM_PADS; i++)
    {
      fPads[i] = new RTVstParam<bool>(add(iParams.fPads[i]));
    }
    // ensures that fSampleSlices is initialized with the correct default values
    fSampleSlices.setNumActiveSlices(*fNumSlices);
    fSampleSlices.setPlayMode(*fPlayModeHold ? EPlayMode::kHold : EPlayMode::kTrigger);
    fSampleSlices.setPolyphonic(*fPolyphonic);
    fSampleSlices.setCrossFade(*fXFade);
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
//    DLOG_F(INFO, "RTState::read - %s", Debug::ParamLine::from(this, true).toString(*iState).c_str());
    //Debug::ParamTable::from(this, true).showCellSeparation().print(*iState, "RTState::read ---> ");
  }

  // beforeWriteNewState
  void beforeWriteNewState(NormalizedState const *iState) override
  {
    // swap the commented line to display either on a line or in a table
//    DLOG_F(INFO, "RTState::write - %s", Debug::ParamLine::from(this, true).toString(*iState).c_str());
    //Debug::ParamTable::from(this, true).showCellSeparation().print(*iState, "RTState::write ---> ");
  }
#endif
};

using namespace GUI;

namespace GUI {
class SampleMgr;
}

//------------------------------------------------------------------------
// SampleSplitterGUIState
//------------------------------------------------------------------------
class SampleSplitterGUIState : public pongasoft::VST::GUI::GUIPluginState<SampleSplitterParameters>, public IErrorHandler
{
public:
  GUIJmbParam<SampleRate> fSampleRate;
  GUIJmbParam<HostInfo> fHostInfo;
  GUIJmbParam<PlayingState> fPlayingState;
  GUIJmbParam<GUI::CurrentSample> fCurrentSample;
  GUIJmbParam<GUI::UndoHistory> fUndoHistory;
  GUIJmbParam<SampleFile> fSampleFile;
  GUIJmbParam<SamplingState> fSamplingState;
  GUIJmbParam<SlicesSettings> fSlicesSettings;
  GUIJmbParam<SampleRange> fWESelectedSampleRange;
  GUIJmbParam<UTF8Path> fLargeFilePath;
  GUIJmbParam<error_message_t> fErrorMessage;
  GUIJmbParam<SharedSampleBuffersMgr32 *> fSharedSampleBuffersMgrPtr;

  std::unique_ptr<SampleMgr> fSampleMgr{};

public:
  explicit SampleSplitterGUIState(SampleSplitterParameters const &iParams);

  // Called when loading or drop of new sample file to maybe load
  // it (ask for confirmation)
  tresult maybeLoadSample(UTF8Path const &iFilePath);

  // Called when loading or drop of new sample file
  tresult loadSample(UTF8Path const &iFilePath);

  void handleError(std::string const &iErrorMessage) override;

  void clearError() override;

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
//      DLOG_F(INFO, "GUIState::write - %s", Debug::ParamLine::from(this, true).toString().c_str());
      //Debug::ParamTable::from(this, true).showCellSeparation().print("GUIState::write ---> ");
    }
    return res;
  }
#endif

};

}
