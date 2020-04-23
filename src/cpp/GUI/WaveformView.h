#pragma once

#include <pongasoft/VST/GUI/Views/CustomView.h>
#include "../Plugin.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI;

/**
 * Base class to handle waveform display
 */
class WaveformView : public Views::StateAwareCustomView<SampleSplitterGUIState>
{
public:
  // Constructor
  explicit WaveformView(const CRect &iSize) : Views::StateAwareCustomView<SampleSplitterGUIState>(iSize)
  {};

  // get/setWaveformColor
  CColor const &getWaveformColor() const { return fWaveformColor; }
  void setWaveformColor(CColor const &iColor) { fWaveformColor = iColor; fBitmap = nullptr; }

  // get/setWaveformAxisColor
  CColor const &getWaveformAxisColor() const { return fWaveformAxisColor; }
  void setWaveformAxisColor(CColor const &iColor) { fWaveformAxisColor = iColor; fBitmap = nullptr; }

  // get/setVerticalSpacing
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

  // handle drag/drop
  bool onDrop(IDataPackage *iDrag, const CPoint &iWhere) override;
  void onDragEnter(IDataPackage *drag, const CPoint &where) override;
  void onDragMove(IDataPackage *drag, const CPoint &where) override;
  void onDragLeave(IDataPackage *drag, const CPoint &where) override;

public:
  CLASS_METHODS_NOCOPY(WaveformView, ToggleButtonView)

protected:
  // onParameterChange
  void onParameterChange(ParamID iParamID) override;

  // generateBitmap
  virtual void generateBitmap(SampleData const &iSampleData) {};

protected:
  CColor fWaveformColor{kWhiteCColor};
  CColor fWaveformAxisColor{kBlackCColor};
  CCoord fVerticalSpacing{};
  Margin fMargin{};

  BitmapSPtr fBitmap{};

  GUIJmbParam<SampleData> fSampleData{};
  GUIJmbParam<SampleRate> fSampleRate{};

public:
  class Creator : public Views::CustomViewCreator<WaveformView, Views::StateAwareCustomView<SampleSplitterGUIState>>
  {
  public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) noexcept :
      CustomViewCreator(iViewName, iDisplayName)
    {
      registerColorAttribute("waveform-color", &WaveformView::getWaveformColor, &WaveformView::setWaveformColor);
      registerColorAttribute("waveform-axis-color", &WaveformView::getWaveformAxisColor, &WaveformView::setWaveformAxisColor);
      registerMarginAttribute("margin", &WaveformView::getMargin, &WaveformView::setMargin);
      registerDoubleAttribute("vertical-spacing", &WaveformView::getVerticalSpacing, &WaveformView::setVerticalSpacing);
    }
  };
};

}

