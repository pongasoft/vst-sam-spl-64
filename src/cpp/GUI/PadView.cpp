#include <vstgui4/vstgui/lib/cdrawcontext.h>

#include "PadView.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

//------------------------------------------------------------------------
// PadView::setEnabled
//------------------------------------------------------------------------
void PadView::setEnabled(bool iEnabled)
{
  if(fEnabled != iEnabled)
  {
    fEnabled = iEnabled;
    markDirty();
  }
}

//------------------------------------------------------------------------
// PadView::draw
//------------------------------------------------------------------------
void PadView::draw(CDrawContext *iContext)
{
  MomentaryButtonView::draw(iContext);
  if(!isEnabled())
  {
    iContext->setFillColor(CColor{0,0,0,120});
    iContext->drawRect(getViewSize(), kDrawFilled);
  }
}

PadView::Creator __gSampleSplitterPadViewCreator("SampleSplitter::Pad", "SampleSplitter - Pad");

}
}
}
}