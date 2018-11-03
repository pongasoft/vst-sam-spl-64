#include "SampleSplitterController.h"
#include "PadController.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

//------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------
SampleSplitterController::SampleSplitterController() : GUIController("SampleSplitter.uidesc"), fParameters{}, fState{fParameters}
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
           fParameters.getGUISaveStateOrder().fVersion,
           Debug::ParamTable::from(getGUIState(), true).keys({Key::kID, Key::kTitle}).full().toString().c_str());
  }
#endif

  return res;
}

//------------------------------------------------------------------------
// SampleSplitterController::onParameterChange
//------------------------------------------------------------------------
void SampleSplitterController::onParameterChange(ParamID iParamID)
{
  if(iParamID == fState.fSampleRate.getParamID())
  {
    DLOG_F(INFO, "Detected sample rate change... %f", fState.fSampleRate.getValue());
    fState.broadcastSample();
  }
}

//------------------------------------------------------------------------
// SampleSplitterController::createCustomController
//------------------------------------------------------------------------
IController *SampleSplitterController::createCustomController(UTF8StringPtr iName, IUIDescription const *iDescription)
{
  if(UTF8StringView(iName) == "PadController")
  {
    DLOG_F(INFO, "SampleSplitterController::createCustomController -> PadController");
    return new PadController();
  }

  return nullptr;
}

}
}
}
}
