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
class SampleEditView : public WaveformView
{
public:
  // Constructor
  explicit SampleEditView(const CRect &iSize) : WaveformView(iSize)
  {};

  // get/setSliceLineColor
  CColor const &getSliceLineColor() const { return fSliceLineColor; }
  void setSliceLineColor(const CColor &iColor) { fSliceLineColor = iColor; }

  // get/setBPMLineColor
  const CColor &getBPMLineColor() const { return fBPMLineColor; }
  void setBPMLineColor(const CColor &iColor) { fBPMLineColor = iColor; }

  // draw
  void draw(CDrawContext *iContext) override;

  // registerParameters
  void registerParameters() override;

  // mouse handling
  CMouseEventResult onMouseDown(CPoint &where, const CButtonState &buttons) override;
  CMouseEventResult onMouseUp(CPoint &where, const CButtonState &buttons) override;
  CMouseEventResult onMouseMoved(CPoint &where, const CButtonState &buttons) override;
  CMouseEventResult onMouseCancel() override;

protected:
  using PixelRange = Range;
  struct Slices;

  // generateBitmap
  void generateBitmap(SampleData const &iSampleData) override;

  void onParameterChange(ParamID iParamID) override;
  void adjustParameters();
  void adjustParametersAfterCut(SampleData::Action const &iCutAction);

  // zoomToSelection
  void zoomToSelection();

  Slices *computeSlices(PixelRange const &iHorizontalRange);

  void initState(GUIState *iGUIState) override;

private:
  CColor fSliceLineColor{kTransparentCColor};
  CColor fBPMLineColor{kTransparentCColor};

  GUIRawVstParam fOffsetPercent{};
  GUIRawVstParam fZoomPercent{};
  GUIVstParam<bool> fShowZeroCrossing{};
  GUIVstParam<int> fNumSlices{};
  GUIJmbParam<HostInfo> fHostInfo{};

  struct RangeEditor;

  // this is denormalized (from fWESelectedSampleRange) because recomputing is expensive
  PixelRange fSelectedPixelRange{-1};
  std::unique_ptr<RangeEditor> fSelectionEditor{};

  // the range of visible samples
  SampleRange fVisibleSampleRange{};

  std::unique_ptr<Slices> fSlices{};

  // keeps a cache of the buffers to avoid loading all the time
  std::unique_ptr<SampleBuffers32> fBuffersCache{};

public:
  class Creator : public Views::CustomViewCreator<SampleEditView, WaveformView>
  {
  public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) noexcept :
      CustomViewCreator(iViewName, iDisplayName)
    {
      registerColorAttribute("slice-line-color", &SampleEditView::getSliceLineColor, &SampleEditView::setSliceLineColor);
      registerColorAttribute("bpm-line-color", &SampleEditView::getBPMLineColor, &SampleEditView::setBPMLineColor);
    }
  };
};

}
}
}
}

