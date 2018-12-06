#include "SampleDisplayView.h"
#include "Waveform.h"
#include <vstgui4/vstgui/lib/coffscreencontext.h>
#include <vstgui4/vstgui/lib/cframe.h>
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

  fSampleData = registerJmbParam(fState->fSampleData, [this]() {
    // recompute the bitmap
    fBitmap = nullptr;
    markDirty();
  });
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
// generateBitmap
//------------------------------------------------------------------------
void SampleDisplayView::generateBitmap(SampleData const &iSampleData)
{
  // TODO: optimization: load -> mono -> compute summary at the same time

  auto buffers = iSampleData.load();
  if(buffers && buffers->hasSamples())
  {
    buffers = buffers->toMono();

    auto context = COffscreenContext::create(getFrame(), getWidth(), getHeight(), getFrame()->getScaleFactor());

    fBitmap = Waveform::createBitmap(context,
                                     buffers.get(),
                                     {getWaveformColor(), 2});
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