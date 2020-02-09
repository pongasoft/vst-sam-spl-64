#include <pongasoft/VST/GUI/Views/CustomView.h>
#include <vstgui4/vstgui/lib/controls/ctextlabel.h>
#include <string>
#include "../Plugin.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace pongasoft::VST::GUI::Views;

/**
 * Displays the slice number (based on offset + bank) */
class SliceNumberView : public StateAwareCustomViewAdapter<CTextLabel, SampleSplitterGUIState>
{
public:
  using super_type = StateAwareCustomViewAdapter<CTextLabel, SampleSplitterGUIState>;

public:
  explicit SliceNumberView(CRect const &iSize) : StateAwareCustomViewAdapter(iSize) {}

  // registerParameters
  void registerParameters() override
  {
    registerCallback<int>(fParams->fPadBank,
                          [this](GUIVstParam<int> &iParam) {
                            if(fSliceOffset >= 0)
                            {
                              auto computedSlice = fSliceOffset + iParam.value() * NUM_PADS;
                              setText(std::to_string(computedSlice));
                            }
                            else
                              setText("");
                          },
                          true);
  }

  int getSliceOffset() const { return fSliceOffset; }
  void setSliceOffset(int offset) { fSliceOffset = offset; }

protected:
  int fSliceOffset{-1};

public:
  // Creator
  class Creator : public CustomViewCreator<SliceNumberView, super_type>
  {
  public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) noexcept :
      CustomViewCreator(iViewName, iDisplayName, VSTGUI::UIViewCreator::kCTextLabel)
    {
      registerIntAttribute("slice-offset",
                           &SliceNumberView::getSliceOffset,
                           &SliceNumberView::setSliceOffset);
    }
  };
};

SliceNumberView::Creator __gSampleSplitterSliceNumberViewCreator("SampleSplitter::SliceNumber", "SampleSplitter - Slice Number");

}