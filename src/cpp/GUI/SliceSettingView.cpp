#include "SliceSettingView.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

//------------------------------------------------------------------------
// SliceSettingView::registerParameters
//------------------------------------------------------------------------
void SliceSettingView::registerParameters()
{
  ToggleButtonView::registerParameters();
  fSelectedSlice = registerVstParam(fParams->fSelectedSlice);
  fSlicesSettings = registerJmbParam(fState->fSlicesSettings);
  setToggleFromSetting();
}

//------------------------------------------------------------------------
// SliceSettingView::setToggleFromSetting
//------------------------------------------------------------------------
void SliceSettingView::setToggleFromSetting()
{
  switch(fType)
  {
    case kReverseSetting:
      ToggleButtonView::setControlValue(fSlicesSettings->isReverse(fSelectedSlice));
      break;

    case kLoopSetting:
      ToggleButtonView::setControlValue(fSlicesSettings->isLoop(fSelectedSlice));
      break;
  }
}

//------------------------------------------------------------------------
// SliceSettingView::setControlValue
//------------------------------------------------------------------------
void SliceSettingView::setControlValue(const bool &iControlValue)
{
  ToggleButtonView::setControlValue(iControlValue);
  switch(fType)
  {
    case kReverseSetting:
      fSlicesSettings.broadcast(fSlicesSettings->reverse(fSelectedSlice, iControlValue));
      break;

    case kLoopSetting:
      fSlicesSettings.broadcast(fSlicesSettings->loop(fSelectedSlice, iControlValue));
      break;
  }
}

//------------------------------------------------------------------------
// SliceSettingView::onParameterChange
//------------------------------------------------------------------------
void SliceSettingView::onParameterChange(ParamID iParamID)
{
  setToggleFromSetting();
  ToggleButtonView::onParameterChange(iParamID);
}

SliceSettingView::Creator __gSampleSplitterSliceSettingViewCreator("SampleSplitter::SliceSetting", "SampleSplitter - Slice Setting");

}
}
}
}