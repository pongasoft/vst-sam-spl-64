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
  VstParam<NumSlices> fNumSlices;
  VstParam<int> fPadBank;
  VstParam<bool> fPads[NUM_PADS];

  JmbParam<double> fSampleRate;
  JmbParam<SampleBuffers32> fFileSample;

public:
  SampleSplitterParameters()
  {
    // the number of slices the sample will be split into
    fNumSlices =
      vst<NumSlicesParamConverter>(ESampleSplitterParamID::kNumSlices, STR16("Num Slices"))
        .shortTitle(STR16("Slices"))
        .add();

    // the bank/page on which the pad is being pressed (4 banks of 16 pads => 64 pads)
    fPadBank =
      vst<DiscreteValueParamConverter<NUM_PAD_BANKS - 1>>(ESampleSplitterParamID::kPadBank, STR16("Page"))
        .defaultValue(0)
        .shortTitle(STR16("Page"))
        .add();

    for(int i = 0; i < NUM_PADS; i++)
    {
      // pad 0
      fPads[i] =
        vst<BooleanParamConverter>(ESampleSplitterParamID::kPad1 + i, PAD_TITLES[i])
          .defaultValue(false)
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

    // the file sample
    fFileSample =
      jmb<SampleBuffersSerializer32>(ESampleSplitterParamID::kFileSample, STR16 ("File Sample"))
        .guiOwned()
        .shared()
        .add();

    // RT save state order
    setRTSaveStateOrder(PROCESSOR_STATE_VERSION,
                        fNumSlices,
                        fPadBank);

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
  RTVstParam<NumSlices> fNumSlices;
  RTVstParam<int> fPadBank;
  RTVstParam<bool> *fPads[NUM_PADS];

  RTJmbOutParam<SampleRate> fSampleRate;

  // When a new sample is loaded, the UI will send it to the RT
  RTJmbInParam<SampleBuffers32> fFileSampleMessage;

  SampleBuffers32 fFileSample;
  SampleSlice fSampleSlices[MAX_SLICES];

public:
  explicit SampleSplitterRTState(SampleSplitterParameters const &iParams) :
    RTState(iParams),
    fNumSlices{add(iParams.fNumSlices)},
    fPadBank{add(iParams.fPadBank)},
    fPads{nullptr},
    fSampleRate{addJmbOut(iParams.fSampleRate)},
    fFileSampleMessage{addJmbIn(iParams.fFileSample)},
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
