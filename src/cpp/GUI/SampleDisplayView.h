#pragma once

#include <pongasoft/VST/GUI/Views/CustomView.h>
#include "../Plugin.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI;

/**
 * This class renders the sample currently loaded as a waveform.
 */
class SampleDisplayView : public Views::PluginCustomView<SampleSplitterGUIState>
{
public:
  // Constructor
  explicit SampleDisplayView(const CRect &iSize) : Views::PluginCustomView<SampleSplitterGUIState>(iSize)
  {};

  // get/setWaveformColor
  CColor const &getWaveformColor() const { return fWaveformColor; }
  void setWaveformColor(CColor const &iColor) { fWaveformColor = iColor; }

  // get/setSelectionColor
  CColor const &getSelectionColor() const { return fSelectionColor; }
  void setSelectionColor(CColor const &iColor) { fSelectionColor = iColor; }

  // draw
  void draw(CDrawContext *iContext) override;

  // registerParameters
  void registerParameters() override;

  // setViewSize -> handle resizing to recompute the bitmap
  void setViewSize(const CRect &rect, bool invalid) override;

  // mouse handling
  CMouseEventResult onMouseDown(CPoint &where, const CButtonState &buttons) override;
  CMouseEventResult onMouseUp(CPoint &where, const CButtonState &buttons) override;
  CMouseEventResult onMouseMoved(CPoint &where, const CButtonState &buttons) override;
  CMouseEventResult onMouseCancel() override;

public:
  CLASS_METHODS_NOCOPY(SampleDisplayView, ToggleButtonView)

protected:
  // generateBitmap
  void generateBitmap(SampleData const &iSampleData);

  // computeSelectedSlice
  int computeSelectedSlice(CPoint const &iWhere) const;

private:
  CColor fWaveformColor{kWhiteCColor};
  CColor fSelectionColor{255, 255, 255, 100};

  BitmapSPtr fBitmap{};

  GUIVstParam<int> fNumSlices{};
  GUIVstParam<int> fSelectedSlice{};
  GUIJmbParam<SampleData> fSampleData{};

  GUIVstParamEditor<int> fSelectedSliceEditor{nullptr};

public:
  class Creator : public Views::CustomViewCreator<SampleDisplayView, Views::PluginCustomView<SampleSplitterGUIState>>
  {
  public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) noexcept :
      CustomViewCreator(iViewName, iDisplayName)
    {
      registerColorAttribute("waveform-color", &SampleDisplayView::getWaveformColor, &SampleDisplayView::setWaveformColor);
      registerColorAttribute("selection-color", &SampleDisplayView::getSelectionColor, &SampleDisplayView::setSelectionColor);
    }
  };
};

}
}
}
}

