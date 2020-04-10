#pragma once

#include "WaveformView.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI;

/**
 * This class renders the sample currently loaded as a waveform.
 */
class SampleDisplayView : public WaveformView
{
public:
  // Constructor
  explicit SampleDisplayView(const CRect &iSize) : WaveformView(iSize)
  {};

  // get/setSelectionColor
  CColor const &getSelectionColor() const { return fSelectionColor; }
  void setSelectionColor(CColor const &iColor) { fSelectionColor = iColor; }

  // draw
  void draw(CDrawContext *iContext) override;

  // registerParameters
  void registerParameters() override;

  // mouse handling
  CMouseEventResult onMouseDown(CPoint &where, const CButtonState &buttons) override;
  CMouseEventResult onMouseUp(CPoint &where, const CButtonState &buttons) override;
  CMouseEventResult onMouseMoved(CPoint &where, const CButtonState &buttons) override;
  CMouseEventResult onMouseCancel() override;

  //------------------------------------------------------------------------
  // slice line color
  //------------------------------------------------------------------------
  CColor const &getSliceLineColor() const { return fSliceLineColor; }
  void setSliceLineColor(const CColor &iColor) { fSliceLineColor = iColor; }

protected:
  // generateBitmap
  void generateBitmap(SampleData const &iSampleData) override;

  // computeSelectedSlice
  int computeSelectedSlice(CPoint const &iWhere) const;

private:
  CColor fSelectionColor{255, 255, 255, 100};
  CColor fSliceLineColor{kTransparentCColor};

  GUIVstParam<NumSlice> fNumSlices{};

  GUIVstParam<int> fSelectedSlice{};
  GUIVstParamEditor<int> fSelectedSliceEditor{nullptr};

public:
  class Creator : public Views::CustomViewCreator<SampleDisplayView, WaveformView>
  {
  public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) noexcept :
      CustomViewCreator(iViewName, iDisplayName)
    {
      registerColorAttribute("selection-color", &SampleDisplayView::getSelectionColor, &SampleDisplayView::setSelectionColor);
      registerColorAttribute("slice-line-color", &SampleDisplayView::getSliceLineColor, &SampleDisplayView::setSliceLineColor);
    }
  };
};

}
}
}
}

