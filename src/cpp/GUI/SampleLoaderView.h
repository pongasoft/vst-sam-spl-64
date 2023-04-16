#pragma once

#include <pongasoft/VST/GUI/Views/TextButtonView.h>
#include "../Plugin.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI;
using namespace pongasoft::VST::GUI::Views;

class SampleLoaderView : public StateAwareView<TextButtonView, SampleSplitterGUIState>
{
public:
  explicit SampleLoaderView(const CRect &iSize) : StateAwareView(iSize) {};

  void onClick() override;

public:
  using Creator = Views::CustomViewCreator<SampleLoaderView, TextButtonView>;
};

}

