#include <vstgui4/vstgui/lib/cdrawcontext.h>
#include <pongasoft/VST/GUI/DrawContext.h>

#include "PadView.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

//------------------------------------------------------------------------
// PadView::setSlice
//------------------------------------------------------------------------
void PadView::setSlice(int iSlice, bool iEnabled)
{
  bool dirty = false;

  if(fSlice != iSlice)
  {
    fSlice = iSlice;
    dirty = true;
  }

  if(fEnabled != iEnabled)
  {
    fEnabled = iEnabled;
    dirty = true;
  }

  if(dirty)
    markDirty();
}

//------------------------------------------------------------------------
// PadView::draw
//------------------------------------------------------------------------
void PadView::draw(CDrawContext *iContext)
{
  drawBackColor(iContext);

  auto rdc = pongasoft::VST::GUI::RelativeDrawContext{this, iContext};

  if(isEnabled())
  {
    if(isPlaying())
    {
      drawOn(iContext);

      auto width = getWidth() * fPercentPlayed;
      // handle reverse
      if(width < 0)
        width = getWidth() + width;
//      rdc.fillRect(0, 0, width, getHeight(), CColor{255,255,255,120});
      rdc.drawLine(width, 0, width, getHeight(), kWhiteCColor);
    }
    else
    {
      drawOff(iContext);
    }
  }
  else
  {
    // not enabled
    drawOff(iContext);
    iContext->setFillColor(CColor{0, 0, 0, 120});
    iContext->drawRect(getViewSize(), kDrawFilled);
  }

  if(fSlice == fSelectedSlice)
  {
    rdc.drawRect(rdc.getViewSize(), kWhiteCColor);
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

//------------------------------------------------------------------------
// PadView::registerParameters
//------------------------------------------------------------------------
void PadView::registerParameters()
{
  MomentaryButtonView::registerParameters();
  fSelectedSlice = registerParam(fParams->fSelectedSlice);
}

//------------------------------------------------------------------------
// PadView::initState
//------------------------------------------------------------------------
void PadView::setControlValue(const bool &iControlValue)
{
  if(iControlValue)
    fSelectedSlice.setValue(fSlice);

  TCustomControlView::setControlValue(iControlValue);
}

PadView::Creator __gSampleSplitterPadViewCreator("SampleSplitter::Pad", "SampleSplitter - Pad");

}
}
}
}