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
  CLASS_METHODS_NOCOPY(SampleDisplayView, WaveformView)

protected:
  // generateBitmap
  void generateBitmap(SampleData const &iSampleData) override;

  // computeSelectedSlice
  int computeSelectedSlice(CPoint const &iWhere) const;

private:
  GUIVstParam<int> fNumSlices{};

  GUIVstParam<int> fSelectedSlice{};
  GUIVstParamEditor<int> fSelectedSliceEditor{nullptr};

public:
  class Creator : public Views::CustomViewCreator<SampleDisplayView, WaveformView>
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

