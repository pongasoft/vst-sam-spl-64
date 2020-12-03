#pragma once

#include <pongasoft/VST/GUI/Views/CustomController.h>
#include <pongasoft/VST/GUI/Views/TextButtonView.h>
#include "../Plugin.h"
#include "SampleMgr.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace pongasoft::VST::GUI;

class SampleEditController : public Views::StateAwareCustomController<SampleSplitterGUIState>
{
public:
  // Constructor
  explicit SampleEditController(IController *iBaseController) : Views::StateAwareCustomController<SampleSplitterGUIState>(iBaseController) {}

  ~SampleEditController() override = default;

  CView *verifyView(CView *view, const UIAttributes &attributes, const IUIDescription *description) override;

  void registerParameters() override;

private:
  GUIRawVstParam fOffsetPercent{};
  GUIRawVstParam fZoomPercent{};
  GUIVstParam<NumSlice> fNumSlices{};

protected:
  using ProcessingCallback = std::function<tresult(CurrentSample *)>;

  Views::TextButtonView::OnClickListener processAction(SampleAction::Type iActionType);

  void initButton(Views::TextButtonView *iButton, SampleAction::Type iActionType, bool iEnabledOnSelection);

  void undoLastAction();

  SampleAction createAction(SampleAction::Type iActionType) const;

  int32 computeSliceSizeInSamples() const;
  NumSlice computeNumSlices(int32 iSliceSizeInSamples) const;
};

}