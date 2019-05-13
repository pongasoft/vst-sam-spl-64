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

  // get/set progress bar margin
  Margin const &getProgressBarMargin() const { return fProgressBarMargin; }
  void setProgressBarMargin(Margin  const &iMargin) { fProgressBarMargin = iMargin; }

  // get/set progress bar color
  const CColor &getProgressBarColor() const { return fProgressBarColor; }
  void setProgressBarColor(const CColor &iColor) { fProgressBarColor = iColor; }

  //------------------------------------------------------------------------
  // draw
  //------------------------------------------------------------------------
  void draw(CDrawContext *iContext) override
  {
    TextButtonView::draw(iContext);

    auto percentSampled = fState->fSamplingState->fPercentSampled;

    if(percentSampled > 0 && percentSampled <= 1.0)
    {
      auto rdc = pongasoft::VST::GUI::RelativeDrawContext{this, iContext};
      auto width = getWidth() * percentSampled;
      rdc.fillRect(0 + fProgressBarMargin.fLeft,
                   0 + fProgressBarMargin.fTop,
                   width - fProgressBarMargin.fRight,
                   getHeight() - fProgressBarMargin.fBottom,
                   fProgressBarColor);
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

protected:
  Margin fProgressBarMargin{};
  CColor fProgressBarColor{255,255,255,120};

public:
  class Creator : public Views::CustomViewCreator<SamplingButtonView, Views::TextButtonView>
  {
  public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) noexcept :
      CustomViewCreator(iViewName, iDisplayName)
    {
      registerMarginAttribute("progress-bar-margin",
                              &SamplingButtonView::getProgressBarMargin,
                              &SamplingButtonView::setProgressBarMargin);
      registerColorAttribute("progress-bar-color",
                              &SamplingButtonView::getProgressBarColor,
                              &SamplingButtonView::setProgressBarColor);
    }

  };
};

// the creator
SamplingButtonView::Creator __gSampleSplitterSamplingButtonCreator("SampleSplitter::SamplingButtonView", "SampleSplitter - SamplingButton");

}
}
}
}

