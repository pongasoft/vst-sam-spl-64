#pragma once

#include <pongasoft/VST/GUI/Views/CustomController.h>
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

  void registerParameters() override;

  CView *verifyView(CView *view, const UIAttributes &attributes, const IUIDescription *description) override;

};

}
}
}
}