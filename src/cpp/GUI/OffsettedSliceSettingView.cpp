#include "../Plugin.h"
#include "SliceSettingView.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace pongasoft::VST::GUI::Views;

/**
 * Toggle button for a setting (reverse or loop) for the slice defined by an offset and a pad bank.
 *
 * Ex: Offset 0 Bank A => Slice 0, Offset 15 Bank D => Slice 63
 */
class OffsettedSliceSettingView : public SliceSettingView
{
public:
  explicit OffsettedSliceSettingView(CRect const &iSize) : SliceSettingView(iSize) {}

  void registerParameters() override
  {
    fPadBank = registerCallback(fParams->fPadBank, [this]() { computeSlice(); setToggleFromSetting(); });
    fSelectedSlice = registerParam(fParams->fSelectedSlice, false);
    computeSlice();
    SliceSettingView::registerParameters();
  }

  int getSlice() override { return fComputedSlice; };

  int getSliceOffset() const { return fSliceOffset; }
  void setSliceOffset(int offset) { fSliceOffset = offset; }

  // Overriding to select the slice when clicking it
  CMouseEventResult onMouseDown(CPoint &where, const CButtonState &buttons) override
  {
    // we select the slice when clicked
    fSelectedSlice = getSlice();
    return ToggleButtonView::onMouseDown(where, buttons);
  }

protected:
  void computeSlice()
  {
    if(fSliceOffset >= 0)
      fComputedSlice = fSliceOffset + *fPadBank * NUM_PADS;
  }

protected:
  GUIVstParam<int> fPadBank{};
  GUIVstParam<int> fSelectedSlice{};
  int fSliceOffset{-1};

  int fComputedSlice{-1};

public:
  // Creator
  class Creator : public CustomViewCreator<OffsettedSliceSettingView, SliceSettingView>
  {
  public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) noexcept :
      CustomViewCreator(iViewName, iDisplayName)
    {
      registerIntAttribute("slice-offset",
                           &OffsettedSliceSettingView::getSliceOffset,
                           &OffsettedSliceSettingView::setSliceOffset);
    }
  };
};

OffsettedSliceSettingView::Creator __gSampleSplitterOffsettedSliceSettingViewSliceSettingViewCreator("SampleSplitter::OffsettedSliceSetting", "SampleSplitter - Offsetted Slice Setting");

}