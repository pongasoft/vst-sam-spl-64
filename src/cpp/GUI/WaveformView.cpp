#include "WaveformView.h"
#include "Waveform.h"
#include <vstgui4/vstgui/lib/coffscreencontext.h>
#include <vstgui4/vstgui/lib/cframe.h>
#include <vstgui4/vstgui/lib/idatapackage.h>
#include <pongasoft/VST/GUI/DrawContext.h>

namespace pongasoft::VST::SampleSplitter::GUI {

//------------------------------------------------------------------------
// WaveformView::registerParameters
//------------------------------------------------------------------------
void WaveformView::registerParameters()
{
  fSampleData = registerParam(fState->fSampleData);
  fSampleRate = registerParam(fState->fSampleRate);
}

//------------------------------------------------------------------------
// WaveformView::draw
//------------------------------------------------------------------------
void WaveformView::draw(CDrawContext *iContext)
{
  CustomView::draw(iContext);

  if(!fBitmap && fSampleData->exists())
    generateBitmap(fSampleData.getValue());
}

//------------------------------------------------------------------------
// WaveformView::setViewSize
//------------------------------------------------------------------------
void WaveformView::setViewSize(const CRect &rect, bool invalid)
{
  if(getViewSize().getSize() != rect.getSize())
    fBitmap = nullptr;

  CView::setViewSize(rect, invalid);
}

//------------------------------------------------------------------------
// WaveformView::onParameterChange
//------------------------------------------------------------------------
void WaveformView::onParameterChange(ParamID iParamID)
{
  if(iParamID == fSampleData.getParamID() || iParamID == fSampleRate.getParamID())
    fBitmap = nullptr;

  CustomView::onParameterChange(iParamID);
}

namespace internal {

//------------------------------------------------------------------------
// internal::findFilePath
//------------------------------------------------------------------------
char const* findFilePath(IDataPackage *iDrag)
{
  for(uint32_t i = 0; i < iDrag->getCount(); i++)
  {
    void const *buffer;
    IDataPackage::Type type;
    iDrag->getData(i, buffer, type);
    if(type == IDataPackage::kFilePath)
      return static_cast<char const *>(buffer);
  }
  return nullptr;
}

}

//------------------------------------------------------------------------
// WaveformView::onDrop
//------------------------------------------------------------------------
bool WaveformView::onDrop(IDataPackage *iDrag, const CPoint &iWhere)
{
  auto filepath = internal::findFilePath(iDrag);

  if(filepath)
  {
    return fState->loadSample(filepath) == kResultOk;
  }

  return false;
}

//------------------------------------------------------------------------
// WaveformView::onDragEnter
//------------------------------------------------------------------------
void WaveformView::onDragEnter(IDataPackage *iDrag, const CPoint &iWhere)
{
  onDragMove(iDrag, iWhere);
}

//------------------------------------------------------------------------
// WaveformView::onDragLeave
//------------------------------------------------------------------------
void WaveformView::onDragLeave(IDataPackage * /* iDrag */, const CPoint &iWhere)
{
  getFrame()->setCursor(CCursorType::kCursorDefault);
}

//------------------------------------------------------------------------
// WaveformView::onDragMove
//------------------------------------------------------------------------
void WaveformView::onDragMove(IDataPackage *iDrag, const CPoint &iWhere)
{
  CCursorType cursorType = internal::findFilePath(iDrag) ? CCursorType::kCursorCopy : CCursorType::kCursorNotAllowed;
  getFrame()->setCursor(cursorType);
}


}