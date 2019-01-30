#include <pongasoft/VST/GUI/Views/CustomView.h>
#include <pongasoft/VST/GUI/Views/TextButtonView.h>
#include <pongasoft/VST/GUI/DrawContext.h>
#include "../Plugin.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI;

/**
 * Sampling button - enabled/disabled depending on input and show progress when sampling
 */
class SamplingButtonView : public Views::PluginView<Views::TextButtonView, SampleSplitterGUIState>
{
public:
  //------------------------------------------------------------------------
  // Constructor
  //------------------------------------------------------------------------
  explicit SamplingButtonView(const CRect &iSize) : PluginView(iSize) {};

  //------------------------------------------------------------------------
  // Constructor
  //------------------------------------------------------------------------
  void draw(CDrawContext *iContext) override
  {
    TextButtonView::draw(iContext);

    auto percentSampled = fState->fSamplingState->fPercentSampled;

    if(percentSampled > 0 && percentSampled <= 1.0)
    {
      auto rdc = pongasoft::VST::GUI::RelativeDrawContext{this, iContext};
      auto width = getWidth() * percentSampled;
      rdc.fillRect(0, 0, width, getHeight(), CColor{255,255,255,120});
    }
  }

  //------------------------------------------------------------------------
  // registerParameters
  //------------------------------------------------------------------------
  void registerParameters() override
  {
    TextButtonView::registerParameters();

    auto callback = [this](GUIVstParam<ESamplingInput> &iParam) {
      if(iParam.getValue() == ESamplingInput::kSamplingOff)
      {
        setMouseEnabled(false);
        unClick();
      }
      else
        setMouseEnabled(true);
    };

    registerCallback<ESamplingInput>(fParams->fSamplingInput,
                                     std::move(callback),
                                     true);

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
        setTitle("Waiting... (Click to Cancel)");
      else
      {
        if(percentSampled == 0)
          setTitle("Sample");
        else
          setTitle("Sampling... (Click to Cancel)");
      }
    }
    
    CustomViewAdapter::onParameterChange(iParamID);
  }

public:
  // Creator
  using Creator = Views::CustomViewCreator<SamplingButtonView, Views::TextButtonView>;
};

// the creator
SamplingButtonView::Creator __gSampleSplitterSamplingButtonCreator("SampleSplitter::SamplingButtonView", "SampleSplitter - SamplingButton");

}
}
}
}

