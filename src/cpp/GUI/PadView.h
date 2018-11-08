#pragma once

#include <pongasoft/VST/GUI/Views/MomentaryButtonView.h>
#include "../Plugin.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace pongasoft::VST::GUI::Views;

class PadView : public PluginView<MomentaryButtonView, SampleSplitterGUIState>
{
public:
  explicit PadView(CRect const &iSize) : PluginView(iSize) {}

  inline bool isEnabled() const { return fEnabled; }

  void setSlice(int iSlice, bool iEnabled);

  inline float getPercentPlayed() const { return fPercentPlayed; }
  inline bool isPlaying() const { return getPercentPlayed() > -1; }
  void setPercentPlayed(float iPercentPlayed);

  void draw(CDrawContext *iContext) override;

  void registerParameters() override;

  void setControlValue(const bool &iControlValue) override;

protected:
  bool fEnabled{true};
  float fPercentPlayed{-1};

  int fSlice{-1};
  GUIVstParam<int> fSelectedSlice{};

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
