#include "SampleSplitterController.h"
#include "PadController.h"
#include "SampleEditController.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

//------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------
SampleSplitterController::SampleSplitterController() :
  GUIController("SampleSplitter.uidesc", "main_view"),
  fParameters{},
  fState{fParameters}
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
    DLOG_F(INFO, "Detected new sampling sample %d", fState.fRTSampleMessage->getNumSamples());

    SampleData sampleData;

    if(sampleData.init(fState.fRTSampleMessage.getValue()) == kResultOk)
      fState.fSampleData.setValue(std::move(sampleData));

    // no need for the raw data anymore
    fState.fRTSampleMessage.updateIf([] (auto msg) -> auto { msg->dispose(); return true; });

  });

  // we need access to these parameters in the callback
  fSampling = registerParam(fParameters.fSampling, false);
  fSamplingInput = registerParam(fParameters.fSamplingInput, false);

  // Handle view change
  fViewType = registerCallback(fParameters.fViewType, [this]() {
    DLOG_F(INFO, "Detected new view type %d", fViewType.getValue());

    switch(fViewType.getValue())
    {
      case EViewType::kMainViewType:
        switchToMainView();
        // make sure we stop sampling
        fSamplingInput = ESamplingInput::kSamplingOff;
        fSampling = false;
        break;

      case EViewType::kEditSampleViewType:
        switchToView("sample_edit_view");
        break;
    }
  });
}

//------------------------------------------------------------------------
// SampleSplitterController::createCustomController
//------------------------------------------------------------------------
IController *SampleSplitterController::createCustomController(UTF8StringPtr iName,
                                                              IUIDescription const *iDescription,
                                                              IController *iBaseController)
{
  if(UTF8StringView(iName) == "PadController")
  {
    return new PadController(iBaseController);
  }

  if(UTF8StringView(iName) == "SampleEditController")
  {
    return new SampleEditController(iBaseController);
  }

  return nullptr;
}

}
}
}
}
