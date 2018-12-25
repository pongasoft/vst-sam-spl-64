#pragma once

#include <pongasoft/VST/GUI/Views/CustomController.h>
#include <pongasoft/VST/GUI/Views/TextButtonView.h>
#include "../Plugin.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace pongasoft::VST::GUI;

class SampleEditController : public Views::PluginCustomController<SampleSplitterGUIState>
{
public:
  ~SampleEditController() override = default;

  CView *verifyView(CView *view, const UIAttributes &attributes, const IUIDescription *description) override;

  void registerParameters() override;

private:
  GUIRawVstParam fOffsetPercent{};
  GUIRawVstParam fZoomPercent{};

protected:
  using ProcessingCallback = std::function<tresult(SampleData *)>;

  Views::TextButtonView::OnClickListener processAction(SampleData::Action::Type iActionType);

  void initButton(Views::TextButtonView *iButton, SampleData::Action::Type iActionType, bool iEnabledOnSelection);

  void undoLastAction();

  SampleData::Action createAction(SampleData::Action::Type iActionType) const;

};

}
}
}
}