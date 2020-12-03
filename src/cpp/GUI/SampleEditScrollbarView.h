#pragma once

#include <pongasoft/VST/GUI/Views/ScrollbarView.h>
#include <pongasoft/VST/GUI/Views/CustomViewCreator.h>

#include "../Plugin.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI::Views;


class SampleEditScrollbarView : public StateAwareView<ScrollbarView, SampleSplitterGUIState>
{
public:
  explicit SampleEditScrollbarView(const CRect &iSize) : StateAwareView(iSize) {}

  // get/setSelectionColor
  CColor const &getSelectionColor() const { return fSelectionColor; }
  void setSelectionColor(CColor const &iColor) { fSelectionColor = iColor; }

  void draw(CDrawContext *iContext) override;

protected:
  void drawLeftHandle(CDrawContext *iContext) override;

  void drawRightHandle(CDrawContext *iContext) override;

  void drawScrollbar(CDrawContext *iContext) override;

protected:
  void registerParameters() override;

protected:
  using PixelRange = pongasoft::VST::GUI::Range;

  CColor fSelectionColor{255, 255, 255, 100};

public:
  CLASS_METHODS_NOCOPY(SampleEditScrollbarView, ScrollbarView)

  // Creator
  class Creator : public CustomViewCreator<SampleEditScrollbarView, ScrollbarView>
  {
  public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) noexcept :
      CustomViewCreator(iViewName, iDisplayName)
    {
      registerColorAttribute("selection-color", &SampleEditScrollbarView::getSelectionColor, &SampleEditScrollbarView::setSelectionColor);
    }
  };

};

}


