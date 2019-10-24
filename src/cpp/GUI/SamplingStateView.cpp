#include <pongasoft/VST/GUI/Views/CustomView.h>
#include <pongasoft/VST/GUI/Views/TextButtonView.h>
#include <pongasoft/VST/GUI/DrawContext.h>
#include <vstgui4/vstgui/lib/controls/ctextlabel.h>
#include "../Plugin.h"

namespace pongasoft::VST::SampleSplitter::GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI;

/**
 * Sampling state - display text depending on state of sampling
 */
class SamplingStateView : public Views::StateAwareCustomViewAdapter<VSTGUI::CTextLabel, SampleSplitterGUIState>
{
public:
  using super_type = Views::StateAwareCustomViewAdapter<VSTGUI::CTextLabel, SampleSplitterGUIState>;

public:
  //------------------------------------------------------------------------
  // Constructor
  //------------------------------------------------------------------------
  explicit SamplingStateView(const CRect &iSize) : super_type(iSize) {};

  //------------------------------------------------------------------------
  // registerParameters
  //------------------------------------------------------------------------
  void registerParameters() override
  {
    registerParam(fState->fSamplingState);
  }

  //------------------------------------------------------------------------
  // onParameterChange
  //------------------------------------------------------------------------
  void onParameterChange(ParamID iParamID) override
  {
    if(iParamID == fState->fSamplingState.getParamID())
    {
      auto percentSampled = fState->fSamplingState->fPercentSampled;

      if(percentSampled == PERCENT_SAMPLED_WAITING)
        setText("Waiting...");
      else
      {
        if(percentSampled == 0)
          setText("");
        else
        {
          auto text = String().printf("%2d%%", static_cast<int>(percentSampled * 100));
          setText(text.text8());
        }
      }
    }
    
    CustomViewAdapter::onParameterChange(iParamID);
  }

public:
  class Creator : public Views::CustomViewCreator<SamplingStateView, super_type>
  {
    public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) :
      CustomViewCreator(iViewName, iDisplayName, VSTGUI::UIViewCreator::kCTextLabel)
    {
    }
  };
};

// the creator
SamplingStateView::Creator __gSampleSplitterSamplingStateCreator("SampleSplitter::SamplingStateView", "SampleSplitter - SamplingState");

}

