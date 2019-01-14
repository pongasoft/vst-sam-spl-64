#include "SampleEditView.h"
#include "Waveform.h"
#include <vstgui4/vstgui/lib/coffscreencontext.h>
#include <vstgui4/vstgui/lib/cframe.h>
#include <pongasoft/VST/GUI/DrawContext.h>
#include <pongasoft/Utils/Lerp.h>
#include <pongasoft/VST/SampleRateBasedClock.h>

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

  fOffsetPercent = registerParam(fParams->fWEOffsetPercent);
  fZoomPercent = registerParam(fParams->fWEZoomPercent);
  fShowZeroCrossing = registerParam(fParams->fWEShowZeroCrossing);
  fNumSlices = registerParam(fParams->fNumSlices);
  fHostInfo = registerParam(fState->fHostInfo);
  registerParam(fState->fWESelectedSampleRange);
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

    // make sure that there is at least one line drawn (0.25 seems to be the magic number...)
    auto rect = RelativeRect(fSelectedPixelRange.fFrom, 0,
                             std::max(fSelectedPixelRange.fFrom + 0.25, fSelectedPixelRange.fTo),
                             getHeight());

    if(rdc.getViewSize().rectOverlap(rect))
      rdc.fillRect(rect, CColor{125,125,125,125});
  }
  
  if(fBuffersCache)
  {
    SampleRateBasedClock clock(fState->fSampleRate);
    auto sampleCount = clock.getSampleCountFor1Bar(fHostInfo->fTempo,
                                                   fHostInfo->fTimeSigNumerator,
                                                   fHostInfo->fTimeSigDenominator);

    auto rdc = pongasoft::VST::GUI::RelativeDrawContext{this, iContext};
    auto horizontalRange = rdc.getHorizontalRange();

    for(int i = 0; i <= fVisibleSampleRange.fTo; i += sampleCount)
    {
      if(i < fVisibleSampleRange.fFrom)
        continue;

      auto x = fVisibleSampleRange.mapValue(i, horizontalRange);
      rdc.drawLine(x, 0, x, getHeight(), getBPMLineColor());
    }

    auto sliceSizeInSamples = static_cast<double>(fBuffersCache->getNumSamples()) / fNumSlices;
    auto sliceIndexInSample = sliceSizeInSamples;

    for(int i = 1; i < fNumSlices; i++)
    {
      if(sliceIndexInSample >= fVisibleSampleRange.fFrom)
      {
        auto x = fVisibleSampleRange.mapValue(sliceIndexInSample, horizontalRange);
        rdc.drawLine(x, 0, x, getHeight(), getSliceLineColor());
      }
      sliceIndexInSample += sliceSizeInSamples;
    }
  }

#if EDITOR_MODE

  if(getEditorMode())
  {
    auto rdc = pongasoft::VST::GUI::RelativeDrawContext{this, iContext};
    rdc.debug("SampleRange: [%.3f,%.3f] | PixelRange: [%.3f,%.3f] | Visible: [%.3f,%.3f] | BPM: %f",
              fState->fWESelectedSampleRange->fFrom, fState->fWESelectedSampleRange->fTo,
              fSelectedPixelRange.fFrom, fSelectedPixelRange.fTo,
              fVisibleSampleRange.fFrom, fVisibleSampleRange.fTo,
              fHostInfo->fTempo);

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
                                     {getWaveformColor(),
                                      getWaveformAxisColor(),
                                      getVerticalSpacing(),
                                      getMargin(),
                                      fShowZeroCrossing ? kRedCColor : kTransparentCColor},
                                     fOffsetPercent,
                                     fZoomPercent,
                                     &startOffset,
                                     &endOffset);

    if(fBitmap)
    {
      fVisibleSampleRange.fFrom = startOffset;
      fVisibleSampleRange.fTo = endOffset;
      if(fState->fWESelectedSampleRange->fFrom != -1.0)
        fSelectedPixelRange = fVisibleSampleRange.mapSubRange(fState->fWESelectedSampleRange,
                                                              RelativeView(getViewSize()).getHorizontalRange(),
                                                              false);
      else
        fSelectedPixelRange = PixelRange{-1.0};
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
    fState->fWESelectedSampleRange.update(fVisibleSampleRange);
    fSelectedPixelRange = rv.getHorizontalRange();
  }
  else
    fSelectionEditor = std::make_unique<RangeEditor>(fVisibleSampleRange,
                                                     fState->fWESelectedSampleRange,
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
// SampleEditView::onParameterChange
//------------------------------------------------------------------------
void SampleEditView::onParameterChange(ParamID iParamID)
{
  if(iParamID == fZoomPercent.getParamID() ||
     iParamID == fOffsetPercent.getParamID() ||
     iParamID == fShowZeroCrossing.getParamID())
    fBitmap = nullptr;

  if(iParamID == fSampleData.getParamID())
  {
    DLOG_F(INFO, "New fSampleData - Source = %d, Update Type = %d", fSampleData->getSource(), fSampleData->getUpdateType());

    switch(fSampleData->getUpdateType())
    {
      case SampleData::UpdateType::kNone:
        fZoomPercent = 0;
        fOffsetPercent = 0;
        fState->fWESelectedSampleRange.update(SampleRange{-1.0});
        break;

      case SampleData::UpdateType::kAction:
        adjustParameters();
        break;

      default:
        // do nothing
        break;
    }

    fBuffersCache = nullptr;
  }

  WaveformView::onParameterChange(iParamID);
}

//------------------------------------------------------------------------
// SampleEditView::adjustParameters
//------------------------------------------------------------------------
void SampleEditView::adjustParameters()
{
  auto history = fSampleData->getUndoHistory();
  if(!history)
    return;

  switch(history->fAction.fType)
  {
    case SampleData::Action::Type::kCrop:
    case SampleData::Action::Type::kTrim:
    case SampleData::Action::Type::kResample:
      fZoomPercent = 0;
      fOffsetPercent = 0;
      fState->fWESelectedSampleRange.update(SampleRange{-1.0});
      break;

    case SampleData::Action::Type::kCut:
      adjustParametersAfterCut(history->fAction);
      break;

    default:
      // leave parameters unchanged
      break;
  }
}

//------------------------------------------------------------------------
// SampleEditView::adjustParametersAfterCut
//------------------------------------------------------------------------
void SampleEditView::adjustParametersAfterCut(SampleData::Action const &iCutAction)
{
  if(!fBuffersCache)
    return;

  auto numSamplesCut = iCutAction.fSelectedSampleRange.fTo - iCutAction.fSelectedSampleRange.fFrom;

  auto newNumSamples = fBuffersCache->getNumSamples() - numSamplesCut;

  // after cut we want to leave the left side unchanged and move the right side by the number of samples cut
  auto newVisibleSampleRange = fVisibleSampleRange;
  newVisibleSampleRange.fTo -= numSamplesCut;

  double offsetPercent, zoomPercent;

  if(Waveform::computeFromOffset(static_cast<int32>(newNumSamples),
                                 getWidth(),
                                 {getWaveformColor(),
                                  getWaveformAxisColor(),
                                  getVerticalSpacing(),
                                  getMargin(),
                                  fShowZeroCrossing ? kRedCColor : kTransparentCColor},
                                 static_cast<int32>(newVisibleSampleRange.fFrom),
                                 static_cast<int32>(newVisibleSampleRange.fTo),
                                 offsetPercent,
                                 zoomPercent))
  {
    fOffsetPercent = offsetPercent;
    fZoomPercent = zoomPercent;
    fState->fWESelectedSampleRange.update(SampleRange{-1.0});
  }
}


// the creator
SampleEditView::Creator __gSampleSplitterSampleEditCreator("SampleSplitter::SampleEditView", "SampleSplitter - SampleEditView");

}
}
}
}