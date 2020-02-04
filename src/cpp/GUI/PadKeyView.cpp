#include <vstgui4/vstgui/lib/controls/ctextlabel.h>
#include <pongasoft/VST/GUI/Views/CustomView.h>
#include <pongasoft/VST/GUI/DrawContext.h>
#include "../Plugin.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI;

/**
 * Displays the key associated to the pad
 */
class PadKeyView : public Views::StateAwareCustomViewAdapter<CTextLabel, SampleSplitterGUIState>
{
private:
  using super_type = Views::StateAwareCustomViewAdapter<CTextLabel, SampleSplitterGUIState>;

public:
  //------------------------------------------------------------------------
  // Constructor
  //------------------------------------------------------------------------
  explicit PadKeyView(const CRect &iSize) : super_type(iSize) {};

  //------------------------------------------------------------------------
  // registerParameters
  //------------------------------------------------------------------------
  void registerParameters() override
  {
    fSelectedSlice = registerParam(fParams->fSelectedSlice);
    fRootKey = registerParam(fParams->fRootKey);

    setText(computePadKey());
  }

  //------------------------------------------------------------------------
  // onParameterChange
  //------------------------------------------------------------------------
  void onParameterChange(ParamID iParamID) override
  {
    super_type::onParameterChange(iParamID);

    setText(computePadKey());
  }

private:
  //------------------------------------------------------------------------
  // computePadKey
  //------------------------------------------------------------------------
  UTF8String computePadKey()
  {
    auto rootKey = *fRootKey + *fSelectedSlice;

    if(rootKey < 0 || rootKey >= NUM_ROOT_KEYS)
      return "N/A";
    else
      return UTF8String(String(KEYS.at(rootKey).c_str()).text8());
  }


private:
  GUIVstParam<int> fSelectedSlice{};
  GUIVstParam<RootKey> fRootKey{};

public:
class Creator : public Views::CustomViewCreator<PadKeyView, super_type>
  {
    public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) :
      CustomViewCreator(iViewName, iDisplayName, VSTGUI::UIViewCreator::kCTextLabel)
    {
    }
  };
};

// the creator
PadKeyView::Creator __gKeyRootCreator("SampleSplitter::PadKeyView", "SampleSplitter - PadKey");

}

