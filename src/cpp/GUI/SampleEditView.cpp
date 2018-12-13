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
    auto offsetX = Utils::Lerp<CCoord>::mapValue(fOffsetPercent, 0, 1.0, 0, fBitmap->getWidth() - getWidth());
    fBitmap->draw(iContext, getViewSize(), {offsetX, 0});
  }
}

//------------------------------------------------------------------------
// generateBitmap
//------------------------------------------------------------------------
void SampleEditView::generateBitmap(SampleData const &iSampleData)
{
  auto buffers = iSampleData.load();
  if(buffers && buffers->hasSamples())
  {
    auto width = Utils::Lerp<CCoord>::mapValue(fZoomPercent, 0, 1.0, getWidth(), getWidth() * 2);
    auto context = COffscreenContext::create(getFrame(), width, getHeight(), getFrame()->getScaleFactor());

    fBitmap = Waveform::createBitmap(context,
                                     buffers.get(),
                                     {getWaveformColor(), getVerticalSpacing(), getMargin()});
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
  if(iParamID == fZoomPercent.getParamID())
    fBitmap = nullptr;

  CustomView::onParameterChange(iParamID);
}


// the creator
SampleEditView::Creator __gSampleSplitterSampleEditCreator("SampleSplitter::SampleEditView", "SampleSplitter - SampleEditView");

}
}
}
}