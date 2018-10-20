#pragma once

#include "SampleSplitterCIDs.h"
#include "SampleBuffers.hpp"
#include "SampleSlice.h"

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

//------------------------------------------------------------------------
// SampleSplitterParameters
//------------------------------------------------------------------------
class SampleSplitterParameters : public Parameters
{
public:
  VstParam<bool> fPad1;

  JmbParam<double> fSampleRate;
  JmbParam<SampleBuffers32> fFileSample;

public:
  SampleSplitterParameters()
  {
    // pad 1
    fPad1 =
      vst<BooleanParamConverter>(ESampleSplitterParamID::kPad1, STR16 ("Pad 1"))
        .defaultValue(false)
        .shortTitle(STR16 ("Pad1"))
        .transient()
        .add();

    // the sample rate
    fSampleRate =
      jmb<DoubleParamSerializer>(ESampleSplitterParamID::kSampleRate, STR16 ("Sample Rate"))
        .defaultValue(0)
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

    // RT save state order
    setRTSaveStateOrder(PROCESSOR_STATE_VERSION);

    // GUI save state order
    setGUISaveStateOrder(CONTROLLER_STATE_VERSION,
                         fFileSample);
  }
};

using namespace RT;

//------------------------------------------------------------------------
// SampleSplitterRTState
//------------------------------------------------------------------------
class SampleSplitterRTState : public RTState
{
public:
  RTVstParam<bool> fPad1;

  RTJmbOutParam<SampleRate> fSampleRate;

  // When a new sample is loaded, the UI will send it to the RT
  RTJmbInParam<SampleBuffers32> fFileSampleMessage;

  SampleBuffers32 fFileSample;
  SampleSlice fPad1Slice;

public:
  explicit SampleSplitterRTState(SampleSplitterParameters const &iParams) :
    RTState(iParams),
    fPad1{add(iParams.fPad1)},
    fSampleRate{addJmbOut(iParams.fSampleRate)},
    fFileSampleMessage{addJmbIn(iParams.fFileSample)},
    fFileSample{0},
    fPad1Slice{}
  {
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
  GUIJmbParam<SampleBuffers32> fFileSample;

public:
  explicit SampleSplitterGUIState(SampleSplitterParameters const &iParams) :
    GUIPluginState(iParams),
    fSampleRate{add(iParams.fSampleRate)},
    fFileSample{add(iParams.fFileSample)}
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
