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
// Used to edit the range
//------------------------------------------------------------------------
struct SampleEditView::RangeEditor
{
  RangeEditor(SampleRange const &iVisibleSampleRange,
              GUIJmbParam<SampleRange> &iSelectedSampleRange,
              PixelRange const &iVisiblePixelRange,
              PixelRange &iSelectedPixelRange,
              CCoord iStartValue,
              bool iExtends) :
    fVisibleSampleRange{iVisibleSampleRange},
    fSelectedSampleRange{iSelectedSampleRange},
    fVisiblePixelRange{iVisiblePixelRange},
    fSelectedPixelRange{iSelectedPixelRange},
    fStartValue{fVisiblePixelRange.clamp(iStartValue)}
  {
    if(iExtends && !fSelectedSampleRange->isSingleValue())
    {
      if(fStartValue < fSelectedPixelRange.fTo)
      {
        fSelectedPixelRange.fFrom = fStartValue;
        fStartValue = fSelectedPixelRange.fTo;
      }
      else
      {
        fSelectedPixelRange.fTo = fStartValue;
        fStartValue = fSelectedPixelRange.fFrom;
      }
    }
    else
    {
      fSelectedPixelRange = PixelRange{fStartValue};
    }

    adjustSelectedSampleRange();
  }

  // setValue
  bool setValue(CCoord iValue)
  {
    iValue = fVisiblePixelRange.clamp(iValue);

    auto newRange = iValue < fStartValue ? PixelRange{iValue, fStartValue} : PixelRange{fStartValue, iValue};

    if(newRange != fSelectedPixelRange)
    {
      fSelectedPixelRange = newRange;
      adjustSelectedSampleRange();
      return true;
    }

    return false;
  }

  // commit
  bool commit()
  {
    if(fSelectedPixelRange.isSingleValue())
    {
      fSelectedSampleRange.update(SampleRange{-1});
      fSelectedPixelRange = PixelRange{-1};
      return true;
    }

    return false;
  }

  // rollback
  void rollback()
  {
    fSelectedSampleRange.update(SampleRange{-1});
    fSelectedPixelRange = PixelRange{-1};
  }

private:
  void adjustSelectedSampleRange()
  {
    fSelectedSampleRange.update(fVisiblePixelRange.mapSubRange(fSelectedPixelRange, fVisibleSampleRange, false));
  }

public:
  SampleRange const &fVisibleSampleRange;
  GUIJmbParam<SampleRange> &fSelectedSampleRange;
  PixelRange const fVisiblePixelRange;
  PixelRange &fSelectedPixelRange;
  CCoord fStartValue;
};

//------------------------------------------------------------------------
// SampleEditView::registerParameters
//------------------------------------------------------------------------
void SampleEditView::registerParameters()
{
  WaveformView::registerParameters();

  fOffsetPercent = registerRawVstParam(fParams->fWaveformEditOffsetPercent);
  fZoomPercent = registerRawVstParam(fParams->fWaveformEditZoomPercent);
  registerJmbParam(fState->fSelectedSampleRange);
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

  if(!fSelectedPixelRange.isSingleValue())
  {
    auto rdc = pongasoft::VST::GUI::RelativeDrawContext{this, iContext};
    rdc.fillRect(static_cast<int32>(std::round(fSelectedPixelRange.fFrom)), 0,
                 static_cast<int32>(std::round(fSelectedPixelRange.fTo)), getHeight(), CColor{125,125,125,125});
  }

#if EDITOR_MODE

  if(getEditorMode())
  {
    auto rdc = pongasoft::VST::GUI::RelativeDrawContext{this, iContext};
    rdc.debug("SampleRange: [%f,%f] | PixelRange: [%f,%f]",
              fState->fSelectedSampleRange->fFrom, fState->fSelectedSampleRange->fTo,
              fSelectedPixelRange.fFrom, fSelectedPixelRange.fTo);
  }

#endif
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

    int32 startOffset;
    int32 endOffset;

    fBitmap = Waveform::createBitmap(context,
                                     fBuffersCache.get(),
                                     {getWaveformColor(), getWaveformAxisColor(), getVerticalSpacing(), getMargin()},
                                     fOffsetPercent,
                                     fZoomPercent,
                                     &startOffset,
                                     &endOffset);

    if(fBitmap)
    {
      fVisibleSampleRange.fFrom = startOffset;
      fVisibleSampleRange.fTo = endOffset;
      if(fState->fSelectedSampleRange->fFrom != -1.0)
        fSelectedPixelRange = fVisibleSampleRange.mapSubRange(fState->fSelectedSampleRange,
                                                              RelativeView(getViewSize()).getHorizontalRange(),
                                                              false);
    }
  }
  else
    fBitmap = nullptr;
}

//------------------------------------------------------------------------
// SampleEditView::onMouseDown
//------------------------------------------------------------------------
CMouseEventResult SampleEditView::onMouseDown(CPoint &where, const CButtonState &buttons)
{
  if(!fBuffersCache)
    return kMouseEventNotHandled;

  RelativeView rv(getViewSize());
  RelativeCoord x = rv.fromAbsolutePoint(where).x;

  if(buttons.isDoubleClick())
  {
    fState->fSelectedSampleRange.update(fVisibleSampleRange);
    fSelectedPixelRange = rv.getHorizontalRange();
  }
  else
    fSelectionEditor = std::make_unique<RangeEditor>(fVisibleSampleRange,
                                                     fState->fSelectedSampleRange,
                                                     rv.getHorizontalRange(),
                                                     fSelectedPixelRange,
                                                     x,
                                                     buttons.getModifierState() == CButton::kShift);

  markDirty();

  return kMouseEventHandled;
}

//------------------------------------------------------------------------
// SampleEditView::onMouseMoved
//------------------------------------------------------------------------
CMouseEventResult SampleEditView::onMouseMoved(CPoint &where, const CButtonState &buttons)
{
  if(fSelectionEditor)
  {
    RelativeView rv(getViewSize());
    RelativeCoord x = rv.fromAbsolutePoint(where).x;

    // TODO: Handle going past the screen and scrolling

    if(fSelectionEditor->setValue(x))
      markDirty();

    return kMouseEventHandled;
  }

  return kMouseEventNotHandled;
}

//------------------------------------------------------------------------
// SampleEditView::onMouseUp
//------------------------------------------------------------------------
CMouseEventResult SampleEditView::onMouseUp(CPoint &where, const CButtonState &buttons)
{
  if(fSelectionEditor)
  {
    RelativeView rv(getViewSize());
    RelativeCoord x = rv.fromAbsolutePoint(where).x;

    fSelectionEditor->setValue(x);
    fSelectionEditor->commit();
    fSelectionEditor = nullptr;

    markDirty();

    return kMouseEventHandled;
  }

  return kMouseEventHandled;
}

//------------------------------------------------------------------------
// SampleEditView::onMouseCancel
//------------------------------------------------------------------------
CMouseEventResult SampleEditView::onMouseCancel()
{
  fSelectionEditor->rollback();
  fSelectionEditor = nullptr;
  markDirty();
  return kMouseEventHandled;
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
    if(!fSampleData->hasUndoHistory())
    {
      fOffsetPercent = 0;
      fZoomPercent = 0;
    }
    fState->fSelectedSampleRange.update(SampleRange{-1});
    fSelectedPixelRange = PixelRange{-1};
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