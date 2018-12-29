#pragma once

#include <pongasoft/VST/GUI/Views/TextButtonView.h>
#include "../Plugin.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI;
using namespace pongasoft::VST::GUI::Views;

class SampleLoaderView : public PluginView<TextButtonView, SampleSplitterGUIState>
{
public:
  explicit SampleLoaderView(const CRect &iSize) : PluginView(iSize) {};

  void onClick() override;

  CMessageResult notify(CBaseObject *sender, IdStringPtr message) override;

public:
  using Creator = Views::CustomViewCreator<SampleLoaderView, TextButtonView>;
};

}
}
}
}

