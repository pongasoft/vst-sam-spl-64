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

  void draw(CDrawContext *iContext) override;

  void registerParameters() override;

  void onParameterChange(ParamID iParamID) override;

public:
  CLASS_METHODS_NOCOPY(SampleDisplayView, ToggleButtonView)

protected:
  void generateBitmap(SampleData const &iSampleData);

private:
  BitmapSPtr fBitmap{};
  GUIJmbParam<SampleData> fSampleData{};

public:
  class Creator : public Views::CustomViewCreator<SampleDisplayView, Views::PluginCustomView<SampleSplitterGUIState>>
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

