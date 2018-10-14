#pragma once

#include <pongasoft/VST/GUI/GUIController.h>
#include "../SampleSplitter.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace pongasoft::VST::GUI;

//------------------------------------------------------------------------
// SampleSplitterController - Main GUI Controller
//------------------------------------------------------------------------
class SampleSplitterController : public GUIController
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
  tresult initialize(FUnknown *context) override;

private:
  // The controller gets its own copy of the parameters (defined in Plugin.h)
  SampleSplitterParameters fParameters;

  // The state accessible in the controller and views
  SampleSplitterGUIState fState;
};

}
}
}
}

