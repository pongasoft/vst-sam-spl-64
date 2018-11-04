#include <vstgui4/vstgui/lib/cdrawcontext.h>
#include <pongasoft/VST/GUI/DrawContext.h>

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
  if(fPercentPlayed > -1)
  {
    drawOn(iContext);

    auto rdc = pongasoft::VST::GUI::RelativeDrawContext{this, iContext};
    auto width = getWidth() * fPercentPlayed;
    rdc.fillRect(0, 0, width, getHeight(), CColor{255,255,255,120});
    rdc.drawLine(width, 0, width, getHeight(), kWhiteCColor);
  }
  else
  {
    MomentaryButtonView::draw(iContext);
  }

  if(!isEnabled())
  {
    iContext->setFillColor(CColor{0,0,0,120});
    iContext->drawRect(getViewSize(), kDrawFilled);
  }
}

//------------------------------------------------------------------------
// PadView::setPercentPlayed
//------------------------------------------------------------------------
void PadView::setPercentPlayed(float iPercentPlayed)
{
  if(iPercentPlayed != fPercentPlayed)
  {
    fPercentPlayed = iPercentPlayed;
    markDirty();
  }
}

PadView::Creator __gSampleSplitterPadViewCreator("SampleSplitter::Pad", "SampleSplitter - Pad");

}
}
}
}