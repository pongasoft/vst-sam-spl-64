#pragma once

#include <pongasoft/VST/GUI/Views/ToggleButtonView.h>
#include "../Plugin.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI;

class SampleLoaderView : public Views::ToggleButtonView, Views::PluginAccessor<SampleSplitterGUIState>
{
public:
  explicit SampleLoaderView(const CRect &iSize) : ToggleButtonView(iSize) {};

  void setControlValue(bool const &iControlValue) override;

  CMessageResult notify(CBaseObject *sender, IdStringPtr message) override;

protected:
private:
  void initState(GUIState *iGUIState) override;

public:
  CLASS_METHODS_NOCOPY(SampleLoaderView, ToggleButtonView)

public:
  class Creator : public Views::CustomViewCreator<SampleLoaderView, ToggleButtonView>
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

