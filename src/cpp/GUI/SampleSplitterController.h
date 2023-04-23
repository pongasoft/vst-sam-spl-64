#pragma once

#include <pongasoft/VST/GUI/GUIController.h>
#include <pongasoft/VST/GUI/Params/GUIParamCxAware.h>
#include "../Plugin.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace pongasoft::VST::GUI;

//------------------------------------------------------------------------
// SampleSplitterController - Main GUI Controller
//------------------------------------------------------------------------
class SampleSplitterController : public GUIController, public ParamAware
{
public:
  //------------------------------------------------------------------------
  // UUID() method used to create the controller
  //------------------------------------------------------------------------
  static inline ::Steinberg::FUID UUID() { return SampleSplitterControllerUID; };

  //------------------------------------------------------------------------
  // Factory method used to create the controller
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
  tresult PLUGIN_API initialize(FUnknown *context) override;

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

  // Manages the samples shared between UI and RT (owned by RT)
  SharedSampleBuffersMgr32 *fSharedSampleBuffersMgr{};

  // Current view type
  GUIVstParam<EViewType> fViewType{};
  GUIRawVstParam fOffsetPercent{};
  GUIRawVstParam fZoomPercent{};
  GUIVstParam<ESamplingInput > fSamplingInput{};
  GUIVstParam<EEditingMode> fEditingMode{};
  GUIVstParam<bool> fSampling{};

  // remember previous sampling input to restore it when switching between tabs
  ESamplingInput fPreviousSamplingInput{ESamplingInput::kSamplingInput1};
};

}

