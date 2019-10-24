#pragma once

#include <pongasoft/VST/GUI/Views/ToggleButtonView.h>
#include "../Plugin.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace pongasoft::VST::GUI::Views;

/**
 * Toggle button for a setting (reverse or loop) for the selected slice
 */
class SliceSettingView : public StateAwareView<ToggleButtonView, SampleSplitterGUIState>
{

public:
  enum Type
  {
    kReverseSetting,
    kLoopSetting
  };

public:
  explicit SliceSettingView(CRect const &iSize) : StateAwareView(iSize) {}

  // get/setType
  SliceSettingView::Type getType() const { return fType; }
  void setType(SliceSettingView::Type iType) { fType = iType; }

  void registerParameters() override;

  void onParameterChange(ParamID iParamID) override;

protected:
  void setToggleFromSetting();

protected:
  GUIVstParam<int> fSelectedSlice{};
  GUIJmbParam<SlicesSettings> fSlicesSettings{};

  SliceSettingView::Type fType{};

public:
  CLASS_METHODS_NOCOPY(SliceSettingView, ToggleButtonView)

  // Creator
  class Creator : public CustomViewCreator<SliceSettingView, ToggleButtonView>
  {
  public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) noexcept :
      CustomViewCreator(iViewName, iDisplayName)
    {
      registerIntegerAttribute<Type>("type",
                                     &SliceSettingView::getType,
                                     &SliceSettingView::setType);
    }
  };

};

}