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
  explicit SampleDisplayView(const CRect &iSize) : Views::PluginCustomView<SampleSplitterGUIState>(iSize)
  {};

  // get/setWaveformColor
  CColor const &getWaveformColor() const { return fWaveformColor; }
  void setWaveformColor(CColor const &iColor) { fWaveformColor = iColor; }

  // get/setSelectionColor
  CColor const &getSelectionColor() const { return fSelectionColor; }
  void setSelectionColor(CColor const &iColor) { fSelectionColor = iColor; }

  void draw(CDrawContext *iContext) override;

  void registerParameters() override;

  void onParameterChange(ParamID iParamID) override;

  void setViewSize(const CRect &rect, bool invalid) override;

public:
  CLASS_METHODS_NOCOPY(SampleDisplayView, ToggleButtonView)

protected:
  void generateBitmap(SampleData const &iSampleData);

private:
  CColor fWaveformColor{kWhiteCColor};
  CColor fSelectionColor{255, 255, 255, 100};

  BitmapSPtr fBitmap{};

  GUIVstParam<int> fNumSlices{};
  GUIVstParam<int> fSelectedSlice{};
  GUIJmbParam<SampleData> fSampleData{};

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

