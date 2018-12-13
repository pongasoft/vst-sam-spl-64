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
 * Base class to handle waveform display
 */
class WaveformView : public Views::PluginCustomView<SampleSplitterGUIState>
{
public:
  // Constructor
  explicit WaveformView(const CRect &iSize) : Views::PluginCustomView<SampleSplitterGUIState>(iSize)
  {};

  // get/setWaveformColor
  CColor const &getWaveformColor() const { return fWaveformColor; }
  void setWaveformColor(CColor const &iColor) { fWaveformColor = iColor; fBitmap = nullptr; }

  // get/setSelectionColor
  CColor const &getSelectionColor() const { return fSelectionColor; }
  void setSelectionColor(CColor const &iColor) { fSelectionColor = iColor; fBitmap = nullptr; }

  CCoord getVerticalSpacing() const { return fVerticalSpacing; }
  void setVerticalSpacing(CCoord iVerticalSpacing) { fVerticalSpacing = iVerticalSpacing; fBitmap = nullptr; }

  // margin : whitespace around the waveform
  Margin const &getMargin() const { return fMargin; }
  void setMargin(Margin  const &iMargin) { fMargin = iMargin; fBitmap = nullptr; }

  // draw
  void draw(CDrawContext *iContext) override;

  // registerParameters
  void registerParameters() override;

  // setViewSize -> handle resizing to recompute the bitmap
  void setViewSize(const CRect &rect, bool invalid) override;

public:
  CLASS_METHODS_NOCOPY(WaveformView, ToggleButtonView)

protected:
  // generateBitmap
  virtual void generateBitmap(SampleData const &iSampleData) {};

protected:
  CColor fWaveformColor{kWhiteCColor};
  CColor fSelectionColor{255, 255, 255, 100};
  CCoord fVerticalSpacing{};
  Margin fMargin{};

  BitmapSPtr fBitmap{};

  GUIJmbParam<SampleData> fSampleData{};

public:
  class Creator : public Views::CustomViewCreator<WaveformView, Views::PluginCustomView<SampleSplitterGUIState>>
  {
  public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) noexcept :
      CustomViewCreator(iViewName, iDisplayName)
    {
      registerColorAttribute("waveform-color", &WaveformView::getWaveformColor, &WaveformView::setWaveformColor);
      registerColorAttribute("selection-color", &WaveformView::getSelectionColor, &WaveformView::setSelectionColor);
      registerMarginAttribute("margin", &WaveformView::getMargin, &WaveformView::setMargin);
      registerDoubleAttribute("vertical-spacing", &WaveformView::getVerticalSpacing, &WaveformView::setVerticalSpacing);
    }
  };
};

}
}
}
}

