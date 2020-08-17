#include "../FilePath.h"

#include "WaveformView.h"
#include "Waveform.h"
#include <vstgui4/vstgui/lib/coffscreencontext.h>
#include <vstgui4/vstgui/lib/cframe.h>
#include <vstgui4/vstgui/lib/idatapackage.h>
#include <pongasoft/VST/GUI/DrawContext.h>

#include <sndfile.hh>

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

//------------------------------------------------------------------------
// internal::findValidFile => is there a valid file embedded?
//------------------------------------------------------------------------
char const* findValidFile(IDataPackage *iDrag)
{
  auto filePath = findFilePath(iDrag);
  if(!filePath)
    return nullptr;
  SndfileHandle sndFile(UTF8Path(filePath).toNativePath().c_str());
  return sndFile.rawHandle() ? filePath : nullptr;
}

}

//------------------------------------------------------------------------
// WaveformView::onDragEnter
//------------------------------------------------------------------------
DragOperation WaveformView::onDragEnter(DragEventData data)
{
  fDragOperation = internal::findValidFile(data.drag) ? DragOperation::Copy : DragOperation::None;
  return fDragOperation;
}

//------------------------------------------------------------------------
// WaveformView::onDragMove
//------------------------------------------------------------------------
DragOperation WaveformView::onDragMove(DragEventData data)
{
  return fDragOperation;
}

//------------------------------------------------------------------------
// WaveformView::onDragLeave
//------------------------------------------------------------------------
void WaveformView::onDragLeave(DragEventData data)
{
  fDragOperation = DragOperation::None;
}

//------------------------------------------------------------------------
// WaveformView::onDrop
//------------------------------------------------------------------------
bool WaveformView::onDrop(DragEventData data)
{
  auto filepath = internal::findFilePath(data.drag);

  if(filepath)
  {
    return fState->maybeLoadSample(filepath) == kResultOk;
  }

  fDragOperation = DragOperation::None;

  return true;
}

//------------------------------------------------------------------------
// WaveformView::getDropTarget
//------------------------------------------------------------------------
SharedPointer<VSTGUI::IDropTarget> WaveformView::getDropTarget()
{
  fDragOperation = DragOperation::None;
  return this;
}


}