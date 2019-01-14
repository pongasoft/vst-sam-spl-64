#include <vstgui4/vstgui/lib/coffscreencontext.h>
#include <vstgui4/vstgui/lib/cframe.h>
#include <pongasoft/VST/GUI/DrawContext.h>
#include <pongasoft/VST/GUI/GUIUtils.h>

#include "WaveformView.h"
#include "Waveform.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

using namespace VSTGUI;
using namespace pongasoft::VST::GUI;

/**
 * This class renders the sample currently loaded as a waveform.
 */
class SampleOverviewView : public WaveformView
{
public:
  // Constructor
  explicit SampleOverviewView(const CRect &iSize) : WaveformView(iSize)
  {};

  //------------------------------------------------------------------------
  // slice line color
  //------------------------------------------------------------------------
  CColor const &getSliceLineColor() const { return fSliceLineColor; }
  void setSliceLineColor(const CColor &iColor) { fSliceLineColor = iColor; }

  //------------------------------------------------------------------------
  // BPM line color
  //------------------------------------------------------------------------
  const CColor &getBPMLineColor() const { return fBPMLineColor; }
  void setBPMLineColor(const CColor &iColor) { fBPMLineColor = iColor; }

  //------------------------------------------------------------------------
  // Mono waveform
  //------------------------------------------------------------------------
  bool isMonoWaveform() const { return fMonoWaveform; }
  void setMonoWaveform(bool iMonoWaveform) { fMonoWaveform = iMonoWaveform; }

  //------------------------------------------------------------------------
  // registerParameters
  //------------------------------------------------------------------------
  void registerParameters() override
  {
    WaveformView::registerParameters();

    fNumSlices = registerParam(fParams->fNumSlices);
    fHostInfo = registerParam(fState->fHostInfo);
  }

  //------------------------------------------------------------------------
  // draw
  //------------------------------------------------------------------------
  void draw(CDrawContext *iContext) override
  {
    WaveformView::draw(iContext);
    if(fBitmap)
    {
      fBitmap->draw(iContext, getViewSize());

      auto rdc = pongasoft::VST::GUI::RelativeDrawContext{this, iContext};
      auto horizontalRange = rdc.getHorizontalRange();
      SampleRange visibleRange(0, fNumSamples);

      auto color = getSliceLineColor();
      if(!CColorUtils::isTransparent(color))
      {
        auto sliceSizeInPixels = getWidth() / fNumSlices;

        auto sliceIndex = sliceSizeInPixels;

        for(auto i = 1; i < fNumSlices; i++)
        {
          rdc.drawLine(sliceIndex, 0, sliceIndex, getHeight(), color);
          sliceIndex += sliceSizeInPixels;
        }
      }
    }
  }

protected:
  //------------------------------------------------------------------------
  // generateBitmap
  //------------------------------------------------------------------------
  void generateBitmap(SampleData const &iSampleData) override
  {
    auto buffers = iSampleData.load();
    if(buffers && buffers->hasSamples())
    {
      if(isMonoWaveform())
        buffers = buffers->toMono();

      auto context = COffscreenContext::create(getFrame(), getWidth(), getHeight(), getFrame()->getScaleFactor());

      fBitmap = Waveform::createBitmap(context,
                                       buffers.get(),
                                       {getWaveformColor(), getWaveformAxisColor(), 2, getMargin()});
      fNumSamples = buffers->getNumSamples();
    }
    else
    {
      fBitmap = nullptr;
      fNumSamples = -1;
    }
  }

protected:
  CColor fSliceLineColor{kTransparentCColor};
  CColor fBPMLineColor{kTransparentCColor};
  bool fMonoWaveform{false};

  GUIVstParam<int> fNumSlices{};
  GUIJmbParam<HostInfo> fHostInfo{};

  int32 fNumSamples{-1};

public:
  class Creator : public Views::CustomViewCreator<SampleOverviewView, WaveformView>
  {
  public:
    explicit Creator(char const *iViewName = nullptr, char const *iDisplayName = nullptr) noexcept :
      CustomViewCreator(iViewName, iDisplayName)
    {
      registerColorAttribute("slice-line-color", &SampleOverviewView::getSliceLineColor, &SampleOverviewView::setSliceLineColor);
      registerColorAttribute("bpm-line-color", &SampleOverviewView::getBPMLineColor, &SampleOverviewView::setBPMLineColor);
      registerBooleanAttribute("mono-waveform", &SampleOverviewView::isMonoWaveform, &SampleOverviewView::setMonoWaveform);
    }
  };
};

// the creator
SampleOverviewView::Creator __gSampleSplitterSampleOverviewCreator("SampleSplitter::SampleOverviewView", "SampleSplitter - SampleOverviewView");
}
}
}
}