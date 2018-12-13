#include "WaveformView.h"
#include "Waveform.h"
#include <vstgui4/vstgui/lib/coffscreencontext.h>
#include <vstgui4/vstgui/lib/cframe.h>
#include <pongasoft/VST/GUI/DrawContext.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

//------------------------------------------------------------------------
// WaveformView::registerParameters
//------------------------------------------------------------------------
void WaveformView::registerParameters()
{
  fSampleData = registerJmbParam(fState->fSampleData, [this]() {
    // recompute the bitmap
    fBitmap = nullptr;
    markDirty();
  });
}

//------------------------------------------------------------------------
// WaveformView::draw
//------------------------------------------------------------------------
void WaveformView::draw(CDrawContext *iContext)
{
  CustomView::draw(iContext);

  if(!fBitmap && fSampleData.exists())
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

}
}
}
}