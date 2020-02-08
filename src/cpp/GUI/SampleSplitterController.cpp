#include "SampleSplitterController.h"
#include "PadController.h"
#include "SampleEditController.h"

namespace pongasoft::VST::SampleSplitter::GUI {

//------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------
SampleSplitterController::SampleSplitterController() :
  GUIController("SampleSplitter.uidesc", "main_view"),
  fParams{},
  fState{fParams}
{
  DLOG_F(INFO, "SampleSplitterController()");
}

//------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------
SampleSplitterController::~SampleSplitterController()
{
  DLOG_F(INFO, "~SampleSplitterController()");
}

//------------------------------------------------------------------------
// SampleSplitterController::initialize
//------------------------------------------------------------------------
tresult SampleSplitterController::initialize(FUnknown *context)
{
  tresult res = GUIController::initialize(context);

  DLOG_F(INFO, "SampleSplitterController::initialize");

  //------------------------------------------------------------------------
  // In debug mode this code displays the order in which the GUI parameters
  // will be saved
  //------------------------------------------------------------------------
#ifndef NDEBUG
  if(res == kResultOk)
  {
    using Key = Debug::ParamDisplay::Key;
    DLOG_F(INFO, "GUI Save State - Version=%d --->\n%s",
           fParams.getGUISaveStateOrder().fVersion,
           Debug::ParamTable::from(getGUIState(), true).keys({Key::kID, Key::kTitle}).full().toString().c_str());
  }
#endif

  return res;
}

//------------------------------------------------------------------------
// SampleSplitterController::createCustomController
//------------------------------------------------------------------------
void SampleSplitterController::registerParameters()
{
  // we need to be notified when:
  // there is a new sample rate (GUI does not have access to it otherwise)
  registerCallback(fState.fSampleRate, [this]() {
    DLOG_F(INFO, "Detected sample rate change... %f", fState.fSampleRate.getValue());
    fState.broadcastSample();
  });

  // there is a new sample after the user is done with sampling
  registerCallback(fState.fRTSampleMessage, [this]() {
    
    if(fState.fRTSampleMessage->getNumSamples() <= 0)
      return;

    DLOG_F(INFO, "Detected new sampling sample %d", fState.fRTSampleMessage->getNumSamples());

    fState.fSampleDataMgr.updateIf([this] (SampleDataMgr *iMgr) -> bool
                                {
                                  return iMgr->load(*fState.fRTSampleMessage);
                                });

    // no need for the raw data anymore
    fState.fRTSampleMessage.updateIf([] (auto msg) -> auto { msg->dispose(); return false; });

    // we reset the settings
    fState.fWESelectedSampleRange.resetToDefault();
    fOffsetPercent = fParams.fWEOffsetPercent->fDefaultValue;
    fZoomPercent = fParams.fWEZoomPercent->fDefaultValue;
  });

  // we need access to these parameters in the callback
  fSampling = registerParam(fParams.fSampling, false);
  fSamplingInput = registerParam(fParams.fSamplingInput, false);
  fOffsetPercent = registerParam(fParams.fWEOffsetPercent, false);
  fZoomPercent = registerParam(fParams.fWEZoomPercent, false);

  auto handleSamplingInput = [this]() {
    if(fViewType == EViewType::kEditSampleViewType && fEditingMode == EEditingMode::kEditingSampling)
    {
      if(fSamplingInput == ESamplingInput::kSamplingOff)
      {
        fSamplingInput = fPreviousSamplingInput;
      }
    }
    else
    {
      if(fSamplingInput != ESamplingInput::kSamplingOff)
      {
        // make sure we stop sampling
        fPreviousSamplingInput = *fSamplingInput;
        fSamplingInput = ESamplingInput::kSamplingOff;
        fSampling = false;
      }
    }
  };

  // Handle view change
  fViewType = registerCallback(fParams.fViewType, handleSamplingInput);

  // Handle editing mode change
  fEditingMode = registerCallback(fParams.fEditingMode, handleSamplingInput);
}

//------------------------------------------------------------------------
// SampleSplitterController::createCustomController
//------------------------------------------------------------------------
IController *SampleSplitterController::createCustomController(UTF8StringPtr iName,
                                                              IUIDescription const * /* iDescription */,
                                                              IController *iBaseController)
{
  auto name = UTF8StringView(iName);

  if(name == "PadController")
  {
    return new PadController(iBaseController);
  }

  if(name == "SampleEditController")
  {
    return new SampleEditController(iBaseController);
  }

  return nullptr;
}

}
