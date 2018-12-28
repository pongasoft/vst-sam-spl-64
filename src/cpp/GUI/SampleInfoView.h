#pragma once

#include <vstgui4/vstgui/lib/controls/ctextlabel.h>
#include <pongasoft/VST/GUI/Views/CustomView.h>
#include "../Plugin.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI::Views;

/**
 * This class renders information about the sample.
 */
class SampleInfoView : public PluginCustomViewAdapter<CTextLabel, SampleSplitterGUIState>
{
public:
  // Constructor
  explicit SampleInfoView(const CRect &iSize) : PluginCustomViewAdapter(iSize)
  {};

  void registerParameters() override;

  void onParameterChange(ParamID iParamID) override;

protected:
  void computeInfo();

public:
  class Creator : public TCustomViewCreator<SampleInfoView>
  {
  public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) noexcept :
      TCustomViewCreator(iViewName, iDisplayName, UIViewCreator::kCTextLabel)
    {
    }
  };
};

}
}
}
}