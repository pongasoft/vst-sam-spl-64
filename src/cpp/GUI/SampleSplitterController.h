#pragma once

#include <pongasoft/VST/GUI/GUIController.h>
#include <pongasoft/VST/GUI/Params/GUIParamCxAware.h>
#include "../Plugin.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace pongasoft::VST::GUI;

//------------------------------------------------------------------------
// SampleSplitterController - Main GUI Controller
//------------------------------------------------------------------------
class SampleSplitterController : public GUIController, public Params::GUIParamCxAware
{
public:
  //------------------------------------------------------------------------
  // Factory method used in SampleSplitter_VST3.cpp to create the controller
  //------------------------------------------------------------------------
  static FUnknown *createInstance(void * /*context*/) { return (IEditController *) new SampleSplitterController(); }

public:
  // Constructor
  SampleSplitterController();

  // Destructor -- overridden for debugging purposes only
  ~SampleSplitterController() override;

  // getGUIState
  GUIState *getGUIState() override { return &fState; }

protected:
  // initialize
  tresult initialize(FUnknown *context) override;

  // registerParameters
  void registerParameters() override;


public:
  IController *createCustomController(UTF8StringPtr iName,
                                      IUIDescription const *iDescription,
                                      IController *iBaseController) override;

private:
  // The controller gets its own copy of the parameters (defined in Plugin.h)
  SampleSplitterParameters fParams;

  // The state accessible in the controller and views
  SampleSplitterGUIState fState;

  // Current view type
  GUIVstParam<EViewType> fViewType{};
  GUIRawVstParam fOffsetPercent{};
  GUIRawVstParam fZoomPercent{};
  GUIVstParam<ESamplingInput > fSamplingInput{};
  GUIVstParam<bool> fSampling{};
};

}
}
}
}

