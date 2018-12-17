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

  // draw
  void draw(CDrawContext *iContext) override;

  // registerParameters
  void registerParameters() override;

  // mouse handling
  CMouseEventResult onMouseDown(CPoint &where, const CButtonState &buttons) override;
  CMouseEventResult onMouseUp(CPoint &where, const CButtonState &buttons) override;
  CMouseEventResult onMouseMoved(CPoint &where, const CButtonState &buttons) override;
  CMouseEventResult onMouseCancel() override;

public:
  CLASS_METHODS_NOCOPY(SampleEditView, WaveformView)

protected:
  // generateBitmap
  void generateBitmap(SampleData const &iSampleData) override;

public:
  void onParameterChange(ParamID iParamID) override;

private:
  GUIRawVstParam fOffsetPercent{};
  GUIRawVstParam fZoomPercent{};

  using SampleRange = Range;
  using PixelRange = Range;

  class RangeEditor;

  // the range of sample selected
  SampleRange fSelectedSampleRange{-1};
  // this is denormalized (from fSelectedSampleRange) but recomputing is expensive + due to math rounding error
  // makes the selection box flickers
  PixelRange fSelectedPixelRange{-1};
  std::unique_ptr<RangeEditor> fSelectionEditor{};

  // the range of visible samples
  SampleRange fVisibleSampleRange{};

  // keeps a cache of the buffers to avoid loading all the time
  std::unique_ptr<SampleBuffers32> fBuffersCache{};

public:
  class Creator : public Views::CustomViewCreator<SampleEditView, WaveformView>
  {
  public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) noexcept :
      CustomViewCreator(iViewName, iDisplayName)
    {
    }
  };
};

}
}
}
}

