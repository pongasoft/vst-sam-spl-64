#pragma once

#include <pongasoft/VST/GUI/Views/CustomController.h>
#include <pongasoft/VST/GUI/Views/TextButtonView.h>
#include "../Plugin.h"

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
  using ProcessingCallback = std::function<tresult(SampleData *)>;

  Views::TextButtonView::OnClickListener processAction(SampleDataAction::Type iActionType);

  void initButton(Views::TextButtonView *iButton, SampleDataAction::Type iActionType, bool iEnabledOnSelection);

  void undoLastAction();

  SampleDataAction createAction(SampleDataAction::Type iActionType) const;

  int32 computeSliceSizeInSamples() const;
  NumSlice computeNumSlices(int32 iSliceSizeInSamples) const;
};

}