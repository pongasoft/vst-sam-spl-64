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

protected:
  using ProcessingCallback = std::function<tresult(SampleData *)>;

  Views::TextButtonView::OnClickListener process(ProcessingCallback iProcessingCallback);

};

}
}
}
}