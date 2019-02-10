#include "SampleEditView.h"
#include "Waveform.h"
#include <vstgui4/vstgui/lib/coffscreencontext.h>
#include <vstgui4/vstgui/lib/cframe.h>
#include <vstgui4/vstgui/lib/idatapackage.h>
#include <pongasoft/VST/GUI/DrawContext.h>
#include <pongasoft/Utils/Lerp.h>
#include <pongasoft/VST/SampleRateBasedClock.h>
#include <pongasoft/VST/GUI/Views/GlobalKeyboardHook.h>

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
              RelativeCoord iStartValue,
              bool iExtends) :
    fVisibleSampleRange{iVisibleSampleRange},
    fSelectedSampleRange{iSelectedSampleRange},
    fVisiblePixelRange{iVisiblePixelRange},
    fSelectedPixelRange{iSelectedPixelRange},
    fStartPixelValue{fVisiblePixelRange.clamp(iStartValue)}
  {
    if(iExtends && !fSelectedSampleRange->isSingleValue())
    {
      if(fStartPixelValue < fSelectedPixelRange.fTo)
      {
        fSelectedPixelRange.fFrom = fStartPixelValue;
        fStartSampleValue = fSelectedSampleRange->fTo;
        adjustSelectedSampleRangeFrom(fStartPixelValue);
        fStartPixelValue = fSelectedPixelRange.fTo;
        return;
      }
      else
      {
        fSelectedPixelRange.fTo = fStartPixelValue;
        fStartSampleValue = fSelectedSampleRange->fFrom;
        adjustSelectedSampleRangeTo(fStartPixelValue);
        fStartPixelValue = fSelectedPixelRange.fFrom;
        return;
      }
    }
    else
    {
      fSelectedPixelRange = PixelRange{fStartPixelValue};
    }

    adjustSelectedSampleRange();
    fStartSampleValue = fSelectedSampleRange->fFrom;
  }

  // setValue
  bool setValue(PixelRange::value_type iValue)
  {
    iValue = fVisiblePixelRange.clamp(iValue);

    if(iValue < fStartPixelValue)
    {
      auto newRange = PixelRange{iValue, fStartPixelValue};
      if(newRange != fSelectedPixelRange)
      {
        fSelectedPixelRange = newRange;
        adjustSelectedSampleRangeFrom(iValue);
        return true;
      }
    }
    else
    {
      auto newRange = PixelRange{fStartPixelValue, iValue};
      if(newRange != fSelectedPixelRange)
      {
        fSelectedPixelRange = newRange;
        adjustSelectedSampleRangeTo(iValue);
        return true;
      }
    }

    return false;
  }

  // extendRangeLeft
  bool extendRangeLeft(SampleRange::value_type iSampleValue, PixelRange::value_type iPixelValue)
  {
    auto newRange = PixelRange{iPixelValue, fSelectedPixelRange.fTo};
    if(newRange != fSelectedPixelRange)
    {
      fSelectedPixelRange = newRange;
      fSelectedSampleRange.update(SampleRange{iSampleValue, fSelectedSampleRange->fTo});
      return true;
    }

    return false;
  }

  // extendRangeRight
  bool extendRangeRight(SampleRange::value_type iSampleValue, PixelRange::value_type iPixelValue)
  {
    auto newRange = PixelRange{fSelectedPixelRange.fFrom, iPixelValue};
    if(newRange != fSelectedPixelRange)
    {
      fSelectedPixelRange = newRange;
      fSelectedSampleRange.update(SampleRange{fSelectedSampleRange->fFrom, iSampleValue});
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
      fSelectedSampleRange.broadcast();
      fSelectedPixelRange = PixelRange{-1};
      return true;
    }

    fSelectedSampleRange.broadcast();

    return false;
  }

  // rollback
  void rollback()
  {
    fSelectedSampleRange.update(SampleRange{-1});
    fSelectedPixelRange = PixelRange{-1};
    fSelectedSampleRange.broadcast();
  }

private:
  void adjustSelectedSampleRange()
  {
    fSelectedSampleRange.update(fVisiblePixelRange.mapSubRange(fSelectedPixelRange, fVisibleSampleRange, false));
  }

  void adjustSelectedSampleRangeTo(PixelRange::value_type iTo)
  {
    auto newTo = fVisiblePixelRange.mapValue(iTo, fVisibleSampleRange, false);
    fSelectedSampleRange.update({fStartSampleValue, newTo});
  }

  void adjustSelectedSampleRangeFrom(PixelRange::value_type iFrom)
  {
    auto newFrom = fVisiblePixelRange.mapValue(iFrom, fVisibleSampleRange, false);
    fSelectedSampleRange.update({newFrom, fStartSampleValue});
  }

public:
  SampleRange const &fVisibleSampleRange;
  GUIJmbParam<SampleRange> &fSelectedSampleRange;
  PixelRange const fVisiblePixelRange;
  PixelRange &fSelectedPixelRange;
  PixelRange::value_type fStartPixelValue;
  SampleRange::value_type fStartSampleValue;
};

//------------------------------------------------------------------------
// SampleEditView::Slices
//------------------------------------------------------------------------
struct SampleEditView::Slices
{
  using SamplePixelSlice = std::pair<SampleRange const &, PixelRange const &>;

  // Constructor
  explicit Slices(int iNumSlices)
  {
    fSampleSlices.reserve(static_cast<unsigned long>(iNumSlices));
    fPixelSlices.reserve(static_cast<unsigned long>(iNumSlices));
  }

  // addSlice
  void addSlice(int32 iStart,
                int32 iEnd,
                SampleRange const &iVisibleSampleRange,
                PixelRange const &iVisiblePixelRange)
  {
    auto sampleRange = SampleRange(iStart, iEnd);
    fSampleSlices.emplace_back(sampleRange);
    fPixelSlices.emplace_back(iVisibleSampleRange.mapSubRange(sampleRange, iVisiblePixelRange, false));
  }

  // getPixelSlice => finds the slice containing x
  SamplePixelSlice getPixelSlice(RelativeCoord x) const
  {
    auto it = std::find_if(fPixelSlices.cbegin(),
                           fPixelSlices.cend(),
                           [x] (auto const &iPixelSlice) { return iPixelSlice.contains(x); });

    if(it != fPixelSlices.cend())
    {
      auto idx = it - fPixelSlices.cbegin();
      return {fSampleSlices[idx], *it};
    }
    else
    {
      return {fSampleSlices[0], fPixelSlices[0]};
    }
  }

  // getSliceNearTo => returns the slice where fTo is close to x
  SamplePixelSlice getSliceNearTo(RelativeCoord x) const
  {
    size_t idx = 0;
    RelativeCoord distance = std::numeric_limits<RelativeCoord>::max();
    for(size_t i = 0; i < fPixelSlices.size(); i++)
    {
      auto newDistance = std::abs(x - fPixelSlices[i].fTo);
      if(newDistance < distance)
      {
        idx = i;
        distance = newDistance;
      }
    }

    return {fSampleSlices[idx], fPixelSlices[idx]};
  }

  // getSliceNearTo => returns the slice where fFrom is close to x
  SamplePixelSlice getSliceNearFrom(RelativeCoord x) const
  {
    size_t idx = 0;
    RelativeCoord distance = std::numeric_limits<RelativeCoord>::max();
    for(size_t i = 0; i < fPixelSlices.size(); i++)
    {
      auto newDistance = std::abs(x - fPixelSlices[i].fFrom);
      if(newDistance < distance)
      {
        idx = i;
        distance = newDistance;
      }
    }

    return {fSampleSlices[idx], fPixelSlices[idx]};
  }


  std::vector<SampleRange> fSampleSlices{};
  std::vector<PixelRange> fPixelSlices{};
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
  fPlayingState = registerParam(fState->fPlayingState);
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
    auto rdc = pongasoft::VST::GUI::RelativeDrawContext{this, iContext};
    auto horizontalRange = rdc.getHorizontalRange();

//    SampleRateBasedClock clock(fState->fSampleRate);
//   auto sampleCount = clock.getSampleCountFor1Bar(fHostInfo->fTempo,
//                                                   fHostInfo->fTimeSigNumerator,
//                                                   fHostInfo->fTimeSigDenominator);
//
//    for(int i = 0; i <= fVisibleSampleRange.fTo; i += sampleCount)
//    {
//      if(i < fVisibleSampleRange.fFrom)
//        continue;
//
//      auto x = fVisibleSampleRange.mapValue(i, horizontalRange);
//      rdc.drawLine(x, 0, x, getHeight(), getBPMLineColor());
//    }

    // shows the slices
    auto slices = computeSlices(horizontalRange);
    if(slices)
    {
      for(size_t i = 1; i < slices->fPixelSlices.size(); i++)
      {
        auto x = slices->fPixelSlices[i].fFrom;
        if(x >= 0 && x < getWidth())
          rdc.drawLine(x, 0, x, getHeight(), getSliceLineColor());
      }
    }

    // display a vertical bar showing the start of the selection
    if(fSelectionEditor)
      rdc.drawLine(fSelectionEditor->fStartPixelValue, 0, fSelectionEditor->fStartPixelValue, getHeight(), kWhiteCColor);

    // display a vertical bar showing progress in playing selection
    float percentPlayed = fPlayingState->fWESelectionPercentPlayer;
    if(percentPlayed != PERCENT_PLAYED_NOT_PLAYING)
    {
      RelativeCoord x;
      if(fSelectedPixelRange.isSingleValue())
      {
        // when no selection, the percentage represents a percentage in the full sample buffer
        auto sample = DPLerp::mapValue(percentPlayed, 0.0, 1.0, 0, fBuffersCache->getNumSamples() - 1);
        x = fVisibleSampleRange.mapValue(sample, horizontalRange, false);
      }
      else
      {
        // when there is a selection, then the percentage represents a percentage in the selection
        x = DPLerp::mapValue(percentPlayed, 0.0, 1.0, fSelectedPixelRange.fFrom, fSelectedPixelRange.fTo);
      }
      if(x >= 0 && x <= getWidth())
        rdc.drawLine(x, 0, x, getHeight(), kWhiteCColor);
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
      fSlices = nullptr;
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
    auto slices = computeSlices(rv.getHorizontalRange());
    if(slices)
    {
      auto slice = slices->getPixelSlice(x);
      updateSelectedSampleRange(slice.first);
      fSelectedPixelRange = slice.second;
    }
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

    bool snap = false;

    // use Alt to disable snapping
    if(buttons.getModifierState() != CButton::kAlt)
    {
      auto slices = computeSlices(rv.getHorizontalRange());
      if(slices)
      {
        if(x > fSelectionEditor->fStartPixelValue)
        {
          auto slice = slices->getSliceNearTo(x);
          if(std::abs(x - slice.second.fTo) < 5)
          {
            fSelectionEditor->extendRangeRight(slice.first.fTo, slice.second.fTo);
            snap = true;
          }
        }
        else
        {
          auto slice = slices->getSliceNearFrom(x);
          if(std::abs(x - slice.second.fFrom) < 5)
          {
            fSelectionEditor->extendRangeLeft(slice.first.fFrom, slice.second.fFrom);
            snap = true;
          }
        }
      }
    }

    if(!snap)
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
        updateSelectedSampleRange(SampleRange{-1.0});
        break;

      case SampleData::UpdateType::kAction:
        adjustParameters();
        break;

      default:
        // do nothing
        break;
    }

    fBuffersCache = nullptr;
    fSelectionEditor = nullptr;
  }

  if(iParamID == fNumSlices.getParamID())
    fSlices = nullptr;

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
      updateSelectedSampleRange(SampleRange{-1.0});
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
    updateSelectedSampleRange(SampleRange{-1.0});
  }
}

//------------------------------------------------------------------------
// SampleEditView::computeSlices
//------------------------------------------------------------------------
SampleEditView::Slices *SampleEditView::computeSlices(PixelRange const &iHorizontalRange)
{
  if(!fSlices && fBuffersCache)
  {
    int numSlices = fNumSlices;
    int32 numSamplesPerSlice = fBuffersCache->getNumSamples() / numSlices;

    fSlices = std::make_unique<Slices>(numSlices);

    int32 start = 0;
    for(int i = 0; i < numSlices; i++, start += numSamplesPerSlice)
    {
      fSlices->addSlice(start, start + numSamplesPerSlice - 1, fVisibleSampleRange, iHorizontalRange);
    }

  }

  return fSlices ? fSlices.get() : nullptr;
}

//------------------------------------------------------------------------
// SampleEditView::zoomToSelection
//------------------------------------------------------------------------
void SampleEditView::zoomToSelection()
{
  if(fBuffersCache && !fState->fWESelectedSampleRange->isSingleValue())
  {
    double offsetPercent, zoomPercent;


    if(Waveform::computeFromOffset(fBuffersCache->getNumSamples(),
                                   getWidth(),
                                   {getWaveformColor(),
                                    getWaveformAxisColor(),
                                    getVerticalSpacing(),
                                    getMargin(),
                                    fShowZeroCrossing ? kRedCColor : kTransparentCColor},
                                   static_cast<int32>(fState->fWESelectedSampleRange->fFrom),
                                   static_cast<int32>(fState->fWESelectedSampleRange->fTo),
                                   offsetPercent,
                                   zoomPercent))
    {
      if(offsetPercent == fOffsetPercent && zoomPercent == fZoomPercent)
      {
        fOffsetPercent = 0.0;
        fZoomPercent = 0.0;
      }
      else
      {
        fOffsetPercent = offsetPercent;
        fZoomPercent = zoomPercent;
      }
    }
  }
}

//------------------------------------------------------------------------
// SampleEditView::initState
//------------------------------------------------------------------------
void SampleEditView::initState(GUIState *iGUIState)
{
  PluginView::initState(iGUIState);

  Views::registerGlobalKeyboardHook(this)->onKeyDown([this] (VstKeyCode const &iKeyCode) ->  auto
                                                     {
                                                       if(iKeyCode.character == 'z')
                                                       {
                                                         zoomToSelection();
                                                         return CKeyboardEventResult::kKeyboardEventHandled;
                                                       }

                                                       return CKeyboardEventResult::kKeyboardEventNotHandled;
                                                     });
}

//------------------------------------------------------------------------
// SampleEditView::updateSelectedSampleRange
//------------------------------------------------------------------------
void SampleEditView::updateSelectedSampleRange(SampleRange const &iRange)
{
  if(fState->fWESelectedSampleRange.update(iRange))
    fState->fWESelectedSampleRange.broadcast();
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
// SampleEditView::onDrop
//------------------------------------------------------------------------
bool SampleEditView::onDrop(IDataPackage *iDrag, const CPoint &iWhere)
{
  auto filepath = internal::findFilePath(iDrag);

  if(filepath)
  {
    return fState->loadSample(filepath) == kResultOk;
  }

  return false;
}

//------------------------------------------------------------------------
// SampleEditView::onDragEnter
//------------------------------------------------------------------------
void SampleEditView::onDragEnter(IDataPackage *iDrag, const CPoint &iWhere)
{
  onDragMove(iDrag, iWhere);
}

//------------------------------------------------------------------------
// SampleEditView::onDragLeave
//------------------------------------------------------------------------
void SampleEditView::onDragLeave(IDataPackage * /* iDrag */, const CPoint &iWhere)
{
  getFrame()->setCursor(CCursorType::kCursorDefault);
}

//------------------------------------------------------------------------
// SampleEditView::onDragMove
//------------------------------------------------------------------------
void SampleEditView::onDragMove(IDataPackage *iDrag, const CPoint &iWhere)
{
  CCursorType cursorType = internal::findFilePath(iDrag) ? CCursorType::kCursorCopy : CCursorType::kCursorNotAllowed;
  getFrame()->setCursor(cursorType);
}

// the creator
SampleEditView::Creator __gSampleSplitterSampleEditCreator("SampleSplitter::SampleEditView", "SampleSplitter - SampleEditView");

}
}
}
}