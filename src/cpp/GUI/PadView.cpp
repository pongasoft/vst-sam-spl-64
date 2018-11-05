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
  drawBackColor(iContext);

  if(isEnabled())
  {
    auto rdc = pongasoft::VST::GUI::RelativeDrawContext{this, iContext};

    if(isPlaying())
    {
      drawOn(iContext);

      auto width = getWidth() * fPercentPlayed;
//      rdc.fillRect(0, 0, width, getHeight(), CColor{255,255,255,120});
      rdc.drawLine(width, 0, width, getHeight(), kWhiteCColor);
    }
    else
    {
      drawOff(iContext);
    }

#if EDITOR_MODE
    if(getEditorMode())
    {
      if(isOn())
        rdc.drawRect(rdc.getViewSize(), kWhiteCColor);
    }
#endif
  }
  else
  {
    // not enabled
    drawOff(iContext);
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