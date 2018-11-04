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

  inline bool isEnabled() const { return fEnabled; }
  void setEnabled(bool iEnabled);

  inline float getPercentPlayed() const { return fPercentPlayed; }
  void setPercentPlayed(float iPercentPlayed);

  void draw(CDrawContext *iContext) override;

protected:
  bool fEnabled{true};
  float fPercentPlayed{-1};

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
