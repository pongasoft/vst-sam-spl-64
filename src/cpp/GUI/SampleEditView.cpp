#include "SampleEditView.h"
#include "Waveform.h"
#include <vstgui4/vstgui/lib/coffscreencontext.h>
#include <vstgui4/vstgui/lib/cframe.h>
#include <pongasoft/VST/GUI/DrawContext.h>
#include <pongasoft/Utils/Lerp.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

//------------------------------------------------------------------------
// SampleEditView::registerParameters
//------------------------------------------------------------------------
void SampleEditView::registerParameters()
{
  WaveformView::registerParameters();

  fOffsetPercent = registerRawVstParam(fParams->fWaveformEditOffsetPercent);
  fZoomPercent = registerRawVstParam(fParams->fWaveformEditZoomPercent);
}

//------------------------------------------------------------------------
// SampleEditView::draw
//------------------------------------------------------------------------
void SampleEditView::draw(CDrawContext *iContext)
{
  WaveformView::draw(iContext);

  if(fBitmap)
  {
    fBitmap->draw(iContext, getViewSize());
  }
}

//------------------------------------------------------------------------
// generateBitmap
//------------------------------------------------------------------------
void SampleEditView::generateBitmap(SampleData const &iSampleData)
{
  if(!fBuffersCache)
    fBuffersCache = std::move(iSampleData.load());

  if(fBuffersCache && fBuffersCache->hasSamples())
  {
    auto context = COffscreenContext::create(getFrame(), getWidth(), getHeight(), getFrame()->getScaleFactor());

    fBitmap = Waveform::createBitmap(context,
                                     fBuffersCache.get(),
                                     {getWaveformColor(), getWaveformAxisColor(), getVerticalSpacing(), getMargin()},
                                     fOffsetPercent,
                                     fZoomPercent);
  }
  else
    fBitmap = nullptr;
}

//------------------------------------------------------------------------
// SampleEditView::onMouseDown
//------------------------------------------------------------------------
CMouseEventResult SampleEditView::onMouseDown(CPoint &where, const CButtonState &buttons)
{

  return kMouseEventHandled;
}

//------------------------------------------------------------------------
// SampleEditView::onMouseMoved
//------------------------------------------------------------------------
CMouseEventResult SampleEditView::onMouseMoved(CPoint &where, const CButtonState &buttons)
{
  return kMouseEventNotHandled;
}

//------------------------------------------------------------------------
// SampleEditView::onMouseUp
//------------------------------------------------------------------------
CMouseEventResult SampleEditView::onMouseUp(CPoint &where, const CButtonState &buttons)
{
  return kMouseEventNotHandled;
}

//------------------------------------------------------------------------
// SampleEditView::onMouseCancel
//------------------------------------------------------------------------
CMouseEventResult SampleEditView::onMouseCancel()
{
  return kMouseEventNotHandled;
}

//------------------------------------------------------------------------
// SampleEditView::onMouseCancel
//------------------------------------------------------------------------
void SampleEditView::onParameterChange(ParamID iParamID)
{
  if(iParamID == fZoomPercent.getParamID() || iParamID == fOffsetPercent.getParamID())
    fBitmap = nullptr;

  if(iParamID == fSampleData.getParamID())
  {
    fOffsetPercent = 0;
    fZoomPercent = 0;
    fBuffersCache = nullptr;
  }

  WaveformView::onParameterChange(iParamID);
}


// the creator
SampleEditView::Creator __gSampleSplitterSampleEditCreator("SampleSplitter::SampleEditView", "SampleSplitter - SampleEditView");

}
}
}
}