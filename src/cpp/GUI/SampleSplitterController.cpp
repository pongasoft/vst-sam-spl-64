#include "SampleSplitterController.h"

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

  if(res == kResultOk)
  {
    GUIParamCxAware::initState(getGUIState());
    // we need to be notified when the sample rate changes
    registerJmbParam(fState.fSampleRate);
  }

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

}
}
}
}
