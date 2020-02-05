#include "../Plugin.h"
#include "SliceSettingView.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace pongasoft::VST::GUI::Views;

/**
 * Toggle button for a setting (reverse or loop) for the selected slice
 */
class SelectedSliceSettingView : public SliceSettingView
{
public:
  explicit SelectedSliceSettingView(CRect const &iSize) : SliceSettingView(iSize) {}

  void registerParameters() override
  {
    fSelectedSlice = registerCallback(fParams->fSelectedSlice, [this]() { setToggleFromSetting(); });
    SliceSettingView::registerParameters();
  }

  int getSlice() override { return *fSelectedSlice; };

protected:
  GUIVstParam<int> fSelectedSlice{};

public:
  // Creator
  using Creator = CustomViewCreator<SelectedSliceSettingView, SliceSettingView>;
};

SelectedSliceSettingView::Creator __gSampleSplitterSelectedSliceSettingViewCreator("SampleSplitter::SelectedSliceSetting", "SampleSplitter - Selected Slice Setting");

}