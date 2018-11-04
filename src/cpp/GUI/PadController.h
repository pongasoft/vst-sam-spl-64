#pragma once

#include <pongasoft/VST/GUI/Views/CustomController.h>
#include "../Plugin.h"
#include "PadView.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace pongasoft::VST::GUI;

class PadController : public Views::PluginCustomController<SampleSplitterGUIState>
{
public:
  ~PadController() override = default;

  void registerParameters() override;

  CView *verifyView(CView *view, const UIAttributes &attributes, const IUIDescription *description) override;

protected:
public:
  void onParameterChange(ParamID iParamID) override;

protected:
  void setEnabled(int iPadIndex);

protected:
  GUIVstParam<int> fNumSlices{};
  GUIVstParam<int> fPadBank{};
  GUIJmbParam<PlayingState> fPlayingState{};

  PadView *fPads[NUM_PADS]{};
};

}
}
}
}