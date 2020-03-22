#pragma once

#include <pongasoft/VST/GUI/Views/CustomController.h>
#include "../Plugin.h"
#include "PadView.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace pongasoft::VST::GUI;

class PadController : public Views::StateAwareCustomController<SampleSplitterGUIState>
{
public:
  // Constructor
  explicit PadController(IController *iBaseController) : Views::StateAwareCustomController<SampleSplitterGUIState>(iBaseController) {}

  ~PadController() override = default;

  void registerParameters() override;

  CView *verifyView(CView *view, const UIAttributes &attributes, const IUIDescription *description) override;

protected:
  void onParameterChange(ParamID iParamID) override;

protected:
  void setSlice(int iPadIndex);

protected:
  GUIVstParam<NumSlice> fNumSlices{};
  GUIVstParam<int> fPadBank{};
  GUIVstParam<int> fSelectedSlice{};
  GUIJmbParam<PlayingState> fPlayingState{};

  PadView *fPads[NUM_PADS]{};
};

}