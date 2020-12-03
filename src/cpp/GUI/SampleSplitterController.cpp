#include "SampleSplitterController.h"
#include "PadController.h"
#include "SampleEditController.h"
#include "LargeFileDialogController.h"

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
// SampleSplitterController::registerParameters
//------------------------------------------------------------------------
void SampleSplitterController::registerParameters()
{
  // we initialize the parameters for the manager
  fState.fSampleMgr->registerParameters();

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

  if(name == "LargeFileDialogController")
  {
    return new LargeFileDialogController(iBaseController);
  }

  return nullptr;
}

}
