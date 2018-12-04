#include "SampleDisplayView.h"
#include <vstgui4/vstgui/lib/coffscreencontext.h>
#include <vstgui4/vstgui/lib/cframe.h>
#include <pongasoft/Utils/Lerp.h>
#include <pongasoft/VST/GUI/DrawContext.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

//------------------------------------------------------------------------
// SampleDisplayView::registerParameters
//------------------------------------------------------------------------
void SampleDisplayView::registerParameters()
{
  fNumSlices = registerVstParam(fParams->fNumSlices);
  fSelectedSlice = registerVstParam(fParams->fSelectedSlice);
  fSampleData = registerJmbParam(fState->fSampleData);
}

//------------------------------------------------------------------------
// SampleDisplayView::onParameterChange
//------------------------------------------------------------------------
void SampleDisplayView::onParameterChange(ParamID iParamID)
{
  if(iParamID == fSampleData.getParamID())
    fBitmap = nullptr;

  CustomView::onParameterChange(iParamID);
}

//------------------------------------------------------------------------
// SampleDisplayView::draw
//------------------------------------------------------------------------
void SampleDisplayView::draw(CDrawContext *iContext)
{
  CustomView::draw(iContext);

  if(!fBitmap && fSampleData.exists())
    generateBitmap(fSampleData.getValue());

  if(fBitmap)
  {
    fBitmap->draw(iContext, getViewSize());

    auto rdc = pongasoft::VST::GUI::RelativeDrawContext{this, iContext};

    auto w = getWidth() / fNumSlices;
    auto x = fSelectedSlice * w;

    if(x < getWidth())
      rdc.fillRect(x, 0, x + w, getHeight(), getSelectionColor());
  }
}

//------------------------------------------------------------------------
// computeSummary
//------------------------------------------------------------------------
int32 computeSummary(Sample32 const *iInputSamples,
                     int32 iNumInputSamples,
                     std::vector<Sample32> &oMin,
                     std::vector<Sample32> &oMax,
                     int32 iNumSamplesPerBucket)
{
  auto max = std::numeric_limits<Sample32>::min();
  auto min = std::numeric_limits<Sample32>::max();

  int bucketIndex = 0;

  for(int inputIndex = 0; inputIndex < iNumInputSamples; inputIndex++, bucketIndex++)
  {
    if(bucketIndex == iNumSamplesPerBucket)
    {
      bucketIndex = 0;
      oMin.push_back(min);
      oMax.push_back(max);
      max = std::numeric_limits<Sample32>::min();
      min = std::numeric_limits<Sample32>::max();
    }

    auto sample = iInputSamples[inputIndex];
    min = std::min(min, sample);
    max = std::max(max, sample);
  }

  if(bucketIndex != 0)
  {
    oMin.push_back(min);
    oMax.push_back(max);
  }

  return static_cast<int32>(oMin.size());
}

//------------------------------------------------------------------------
// generateBitmap
//------------------------------------------------------------------------
void SampleDisplayView::generateBitmap(SampleData const &iSampleData)
{
  DLOG_F(INFO, "SampleDisplayView::generateBitmap");

  // TODO: optimization: load -> mono -> compute summary at the same time

  auto buffers = iSampleData.load();
  if(buffers && buffers->hasSamples())
  {
    buffers = buffers->toMono();

    std::vector<Sample32> mins;
    std::vector<Sample32> maxs;

    auto size = computeSummary(buffers->getChannelBuffer(0),
                               buffers->getNumSamples(),
                               mins,
                               maxs,
                               static_cast<int32>(buffers->getNumSamples() / getWidth()));

    if(auto offscreen = COffscreenContext::create(getFrame(), getWidth(), getHeight(), getFrame()->getScaleFactor()))
    {
      // mapping [1,-1] to [0, height] for display
      auto lerp = Utils::Lerp<CCoord>::mapRange(1, -1, 0, getHeight());

      // using antialising and non integral mode to smooth out rendering
      offscreen->setDrawMode(kAntiAliasing | kNonIntegralMode);

      offscreen->beginDraw();
      offscreen->setFrameColor(getWaveformColor());

      // for each x, draw a line connecting min to max
      CPoint p1, p2;
      for(int x = 0; x < size; x++)
      {
        p1.x = x;
        p1.y = lerp.computeY(mins[x]);
        p2.x = x;
        p2.y = lerp.computeY(maxs[x]);

        offscreen->drawLine(p1, p2);
      }

      // draw the main axis to make sure there is no holes
      p1.x = 0; p1.y = getHeight()/2;
      p2.x = getWidth(); p2.y = p1.y;
      offscreen->drawLine(p1, p2);

      offscreen->endDraw();

      // extract the bitmap (using shared pointer so will take care of deleting automatically)
      fBitmap = offscreen->getBitmap();
    }
  }
  else
    fBitmap = nullptr;
}

//------------------------------------------------------------------------
// SampleDisplayView::setViewSize
//------------------------------------------------------------------------
void SampleDisplayView::setViewSize(const CRect &rect, bool invalid)
{
  if(getViewSize().getSize() != rect.getSize())
    fBitmap = nullptr;

  CView::setViewSize(rect, invalid);
}

//------------------------------------------------------------------------
// SampleDisplayView::computeSelectedSlice
//------------------------------------------------------------------------
int SampleDisplayView::computeSelectedSlice(CPoint const &iWhere) const
{
  RelativeView rv(this);

  auto w = getWidth() / fNumSlices;
  auto x = Utils::clamp<CCoord>(rv.fromAbsolutePoint(iWhere).x, 0, getWidth());

  return Utils::clamp(x / w, 0, NUM_SLICES - 1);
}

//------------------------------------------------------------------------
// SampleDisplayView::onMouseDown
//------------------------------------------------------------------------
CMouseEventResult SampleDisplayView::onMouseDown(CPoint &where, const CButtonState &buttons)
{
  fSelectedSliceEditor = fSelectedSlice.edit(computeSelectedSlice(where));

  return kMouseEventHandled;
}

//------------------------------------------------------------------------
// SampleDisplayView::onMouseMoved
//------------------------------------------------------------------------
CMouseEventResult SampleDisplayView::onMouseMoved(CPoint &where, const CButtonState &buttons)
{
  if(fSelectedSliceEditor)
  {
    fSelectedSliceEditor->setValue(computeSelectedSlice(where));
    return kMouseEventHandled;
  }

  return kMouseEventNotHandled;
}

//------------------------------------------------------------------------
// SampleDisplayView::onMouseUp
//------------------------------------------------------------------------
CMouseEventResult SampleDisplayView::onMouseUp(CPoint &where, const CButtonState &buttons)
{
  if(fSelectedSliceEditor)
  {
    fSelectedSliceEditor->commit(computeSelectedSlice(where));
    fSelectedSliceEditor = nullptr;
    return kMouseEventHandled;
  }

  return kMouseEventNotHandled;
}

//------------------------------------------------------------------------
// SampleDisplayView::onMouseCancel
//------------------------------------------------------------------------
CMouseEventResult SampleDisplayView::onMouseCancel()
{
  if(fSelectedSliceEditor)
  {
    fSelectedSliceEditor->rollback();
    fSelectedSliceEditor = nullptr;
    return kMouseEventHandled;
  }

  return kMouseEventNotHandled;
}


// the creator
SampleDisplayView::Creator __gSampleSplitterSampleDisplayCreator("SampleSplitter::SampleDisplayView", "SampleSplitter - SampleDisplayView");

}
}
}
}