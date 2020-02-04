#include "SliceSettingView.h"

namespace pongasoft::VST::SampleSplitter::GUI {

//------------------------------------------------------------------------
// SliceSettingView::registerParameters
//------------------------------------------------------------------------
void SliceSettingView::registerParameters()
{
  ToggleButtonView::registerParameters();
  fSelectedSlice = registerCallback(fParams->fSelectedSlice, [this]() { setToggleFromSetting(); });
  fSlicesSettings = registerCallback(fState->fSlicesSettings, [this]() { setToggleFromSetting() ;});
  setToggleFromSetting();
}

//------------------------------------------------------------------------
// SliceSettingView::setToggleFromSetting
//------------------------------------------------------------------------
void SliceSettingView::setToggleFromSetting()
{
  // implementation note: no need to explicitly call markDirty() because redrawing this view is tied to
  // the state of the toggle which is explicitly set in this method
  switch(fType)
  {
    case kReverseSetting:
      setOnOrOff(fSlicesSettings->isReverse(*fSelectedSlice));
      break;

    case kLoopSetting:
      setOnOrOff(fSlicesSettings->isLoop(*fSelectedSlice));
      break;
  }
}

//------------------------------------------------------------------------
// SliceSettingView::onParameterChange
//------------------------------------------------------------------------
void SliceSettingView::onParameterChange(ParamID iParamID)
{
  // implementation note: this is called only when the toggle changes since the other
  // parameters are using callbacks
  switch(fType)
  {
    case kReverseSetting:
      if(fSlicesSettings.update(fSlicesSettings->reverse(*fSelectedSlice, isOn())))
        fSlicesSettings.broadcast();
      break;

    case kLoopSetting:
      if(fSlicesSettings.update(fSlicesSettings->loop(*fSelectedSlice, isOn())))
        fSlicesSettings.broadcast();
      break;
  }

  ToggleButtonView::onParameterChange(iParamID);
}

SliceSettingView::Creator __gSampleSplitterSliceSettingViewCreator("SampleSplitter::SliceSetting", "SampleSplitter - Slice Setting");

}