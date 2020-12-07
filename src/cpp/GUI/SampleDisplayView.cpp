#include "SampleDisplayView.h"
#include "Waveform.h"
#include <vstgui4/vstgui/lib/coffscreencontext.h>
#include <vstgui4/vstgui/lib/cframe.h>
#include <pongasoft/VST/GUI/DrawContext.h>
#include <pongasoft/VST/GUI/GUIUtils.h>

namespace pongasoft::VST::SampleSplitter::GUI {

//------------------------------------------------------------------------
// SampleDisplayView::registerParameters
//------------------------------------------------------------------------
void SampleDisplayView::registerParameters()
{
  WaveformView::registerParameters();

  fNumSlices = registerParam(fParams->fNumSlices);
  fSelectedSlice = registerParam(fParams->fSelectedSlice);
}

//------------------------------------------------------------------------
// SampleDisplayView::draw
//------------------------------------------------------------------------
void SampleDisplayView::draw(CDrawContext *iContext)
{
  WaveformView::draw(iContext);

  if(fBitmap)
  {
    fBitmap->draw(iContext, getViewSize());

    auto rdc = pongasoft::VST::GUI::RelativeDrawContext{this, iContext};

    if(!GUI::CColorUtils::isTransparent(getSelectionColor()))
    {
      auto w = getWidth() / fNumSlices->realValue();
      auto x = *fSelectedSlice * w;

      if(x < getWidth())
        rdc.fillRect(x, 0, x + w, getHeight(), getSelectionColor());
    }

    // second draw the slices
    auto &color = getSliceLineColor();
    if(!CColorUtils::isTransparent(color))
    {
      auto numPixelsPerSlice = getWidth() / fNumSlices->realValue();

      auto w = numPixelsPerSlice;

      for(int32 i = 1; i < fNumSlices->intValue(); i++)
      {
        rdc.drawLine(w, 0, w, getHeight(), color);
        w += numPixelsPerSlice;
      }
    }
  }
}

//------------------------------------------------------------------------
// generateBitmap
//------------------------------------------------------------------------
void SampleDisplayView::generateBitmap(CurrentSample const &iCurrentSample)
{
  if(iCurrentSample.hasSamples())
  {
    auto context = COffscreenContext::create(getFrame(), getWidth(), getHeight(), getFrame()->getScaleFactor());

    fBitmap = Waveform::createBitmap(context,
                                     iCurrentSample.getBuffers(),
                                     {getWaveformColor(), getWaveformAxisColor(), 2, getMargin()});
  }
  else
    fBitmap = nullptr;
}

//------------------------------------------------------------------------
// SampleDisplayView::computeSelectedSlice
//------------------------------------------------------------------------
int SampleDisplayView::computeSelectedSlice(CPoint const &iWhere) const
{
  RelativeView rv(this);

  auto w = getWidth() / fNumSlices->realValue();
  auto x = Utils::clamp<CCoord>(rv.fromAbsolutePoint(iWhere).x, 0, getWidth());

  return Utils::clamp<int>(x / w, 0, NUM_SLICES - 1);
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