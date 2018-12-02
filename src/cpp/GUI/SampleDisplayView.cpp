#include "SampleDisplayView.h"
#include <vstgui4/vstgui/lib/coffscreencontext.h>
#include <pongasoft/Utils/Lerp.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

//------------------------------------------------------------------------
// SampleDisplayView::registerParameters
//------------------------------------------------------------------------
void SampleDisplayView::registerParameters()
{
  fSampleData = registerJmbParam(fState->fSampleData);
}

//------------------------------------------------------------------------
// SampleDisplayView::onParameterChange
//------------------------------------------------------------------------
void SampleDisplayView::onParameterChange(ParamID iParamID)
{
  if(iParamID == fSampleData.getParamID())
  {
    generateBitmap(fSampleData.getValue());
  }
  CustomView::onParameterChange(iParamID);
}

//------------------------------------------------------------------------
// SampleDisplayView::draw
//------------------------------------------------------------------------
void SampleDisplayView::draw(CDrawContext *iContext)
{
  CustomView::draw(iContext);

  if(fBitmap)
    fBitmap->draw(iContext, getViewSize());
}

//------------------------------------------------------------------------
// generateBitmap
//------------------------------------------------------------------------
void SampleDisplayView::generateBitmap(SampleData const &iSampleData)
{
  DLOG_F(INFO, "SampleDisplayView::generateBitmap NOT IMPLEMENTED");
//  auto buffers = iSampleData.load();
//  if(buffers && buffers->hasSamples())
//  {
//    const auto numSamples = buffers->getNumSamples();
//    auto mono = std::make_unique<SampleBuffers32>(buffers->getSampleRate(), 1, numSamples);
//
//    auto monoBuffer = mono->getChannelBuffer(0);
//    for(int i = 0; i < numSamples; i++)
//    {
//      monoBuffer[i] = 0;
//    }
//
//    for(int c = 0; c < buffers->getNumChannels(); c++)
//    {
//      Sample32 *buffer = buffers->getChannelBuffer(c);
//      for(int i = 0; i < numSamples; i++)
//      {
//        monoBuffer[i] += buffer[i];
//      }
//    }
//
//    auto computedSampleRate = getWidth() * mono->getSampleRate() / numSamples;
//    mono = mono->resample(computedSampleRate);
//
//    auto buffer = mono->getChannelBuffer(0);
//    if(auto offscreen = COffscreenContext::create(getFrame(), getWidth(), getHeight()))
//    {
//      auto lerp = Utils::Lerp<CCoord>::mapRange(1, -1, 0, getHeight());
//      offscreen->beginDraw();
//      for(int i = 0; i < mono->getNumSamples(); i++)
//      {
//        offscreen->drawPoint(CPoint{static_cast<CCoord>(i), lerp.computeY(buffer[i]) }, kWhiteCColor);
//      }
//      offscreen->endDraw();
//      fBitmap = offscreen->getBitmap();
//    }
//  }

}

// the creator
SampleDisplayView::Creator __gSampleSplitterSampleDisplayCreator("SampleSplitter::SampleDisplayView", "SampleSplitter - SampleDisplayView");

}
}
}
}