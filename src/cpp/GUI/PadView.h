#pragma once

#include <pongasoft/VST/GUI/Views/MomentaryButtonView.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace pongasoft::VST::GUI::Views;

class PadView : public MomentaryButtonView
{
public:
  explicit PadView(CRect const &iSize) : MomentaryButtonView(iSize) {}

  bool isEnabled() const { return fEnabled; }
  void setEnabled(bool iEnabled);

  void draw(CDrawContext *iContext) override;

protected:
  bool fEnabled{true};

public:
  CLASS_METHODS_NOCOPY(PadView, MomentaryButtonView)

  // Creator
  class Creator : public CustomViewCreator<PadView, MomentaryButtonView>
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
