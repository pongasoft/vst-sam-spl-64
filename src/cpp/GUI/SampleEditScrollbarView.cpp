#include "SampleEditScrollbarView.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

//------------------------------------------------------------------------
// SampleEditScrollbarView::registerParameters
//------------------------------------------------------------------------
void SampleEditScrollbarView::registerParameters()
{
  ScrollbarView::registerParameters();
  registerParam(fState->fWESelectedSampleRange);
}

//------------------------------------------------------------------------
// SampleEditScrollbarView::draw
//------------------------------------------------------------------------
void SampleEditScrollbarView::draw(CDrawContext *iContext)
{

  if(!fState->fWESelectedSampleRange->isSingleValue())
  {
    SampleInfo sampleInfo;
    if(fState->fSampleData->getSampleInfo(sampleInfo) == kResultOk)
    {
      recompute();

      PixelRange fullRange(fZoomBox.getMinLeft() + fScrollbarGutterSpacing + fLeftHandleRect.getWidth(),
                           fZoomBox.getMaxRight() - fScrollbarGutterSpacing - fRightHandleRect.getWidth());


      auto range =
        SampleRange(0, sampleInfo.fNumSamples).mapSubRange(fState->fWESelectedSampleRange, fullRange);

      // make sure that there is at least one line drawn (0.25 seems to be the magic number...)
      range.fTo = std::max(range.fFrom + 0.25, range.fTo);

      auto rdc = pongasoft::VST::GUI::RelativeDrawContext{this, iContext};

      rdc.fillRect(range.fFrom, 0, range.fTo, getHeight(), getSelectionColor());
    }

  }

  ScrollbarView::draw(iContext);
}

//------------------------------------------------------------------------
// SampleEditScrollbarView::drawLeftHandle
//------------------------------------------------------------------------
void SampleEditScrollbarView::drawLeftHandle(CDrawContext *iContext)
{
  iContext->setFillColor(fZoomHandlesColor);
  iContext->drawRect(fLeftHandleRect, kDrawFilled);
}

//------------------------------------------------------------------------
// SampleEditScrollbarView::drawRightHandle
//------------------------------------------------------------------------
void SampleEditScrollbarView::drawRightHandle(CDrawContext *iContext)
{
  iContext->setFillColor(fZoomHandlesColor);
  iContext->drawRect(fRightHandleRect, kDrawFilled);
}

//------------------------------------------------------------------------
// SampleEditScrollbarView::drawScrollbar
//------------------------------------------------------------------------
void SampleEditScrollbarView::drawScrollbar(CDrawContext *iContext)
{
  ScrollbarView::drawScrollbar(iContext);
}

// the creator
SampleEditScrollbarView::Creator __gSampleSplitterSampleEditScrollbarCreator("SampleSplitter::SampleEditScrollbarView", "SampleSplitter - SampleEditScrollbarView");

}
}
}
}
