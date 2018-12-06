#pragma once

#include <pongasoft/VST/GUI/Views/TextButtonView.h>
#include "../Plugin.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI;

class SampleLoaderView : public Views::TextButtonView, Views::PluginAccessor<SampleSplitterGUIState>
{
public:
  explicit SampleLoaderView(const CRect &iSize) : TextButtonView(iSize) {};

  void onClick() override;

  CMessageResult notify(CBaseObject *sender, IdStringPtr message) override;

protected:
  void initState(GUIState *iGUIState) override;

public:
  CLASS_METHODS_NOCOPY(SampleLoaderView, Views::TextButtonView)

public:
  class Creator : Views::CustomViewCreator<SampleLoaderView, TextButtonView>
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

