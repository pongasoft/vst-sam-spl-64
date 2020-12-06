#include "SampleEditController.h"

#include <vstgui4/vstgui/lib/controls/ctextlabel.h>

namespace pongasoft::VST::SampleSplitter::GUI {

//------------------------------------------------------------------------
// SampleEditController::verifyView
//------------------------------------------------------------------------
CView *SampleEditController::verifyView(CView *iView,
                                        const UIAttributes &iAttributes,
                                        const IUIDescription * /* iDescription */)
{
  auto button = dynamic_cast<Views::TextButtonView *>(iView);

  if(button)
  {
    switch(button->getCustomViewTag())
    {
      case ESampleSplitterParamID::kNormalize0Action:
        initButton(button, SampleAction::Type::kNormalize0, false);
        break;

      case ESampleSplitterParamID::kNormalize3Action:
        initButton(button, SampleAction::Type::kNormalize3, false);
        break;

      case ESampleSplitterParamID::kNormalize6Action:
        initButton(button, SampleAction::Type::kNormalize6, false);
        break;

      case ESampleSplitterParamID::kTrimAction:
        initButton(button, SampleAction::Type::kTrim, false);
        break;

      case ESampleSplitterParamID::kCutAction:
        initButton(button, SampleAction::Type::kCut, true);
        break;

      case ESampleSplitterParamID::kCropAction:
        initButton(button, SampleAction::Type::kCrop, true);
        break;

      case ESampleSplitterParamID::kUndoAction:
      {
        // sets a listener to handle undo click
        button->setOnClickListener([this] { fState->fSampleMgr->undoLastAction(); });

        // enable/disable the button based on whether there is an undo history
        auto callback = [] (Views::TextButtonView *iButton, GUIJmbParam<UndoHistory> &iParam) {
          iButton->setMouseEnabled(iParam->hasUndoHistory());
        };

        makeParamAware(button)->registerCallback<UndoHistory>(fState->fUndoHistory,
                                                              std::move(callback),
                                                              true);
        break;
      }

      case ESampleSplitterParamID::kRedoAction:
      {
        // sets a listener to handle redo click
        button->setOnClickListener([this] { fState->fSampleMgr->redoLastUndo(); });

        // enable/disable the button based on whether there is an redo history
        auto callback = [] (Views::TextButtonView *iButton, GUIJmbParam<UndoHistory> &iParam) {
          iButton->setMouseEnabled(iParam->hasRedoHistory());
        };

        makeParamAware(button)->registerCallback<UndoHistory>(fState->fUndoHistory,
                                                              std::move(callback),
                                                              true);
        break;
      }

      case ESampleSplitterParamID::kClearHistoryAction:
      {
        // sets a listener to handle clearing history click
        button->setOnClickListener([this] {
          fState->fUndoHistory.updateIf([] (UndoHistory *iHistory) {
            return iHistory->clearActionHistory();
          });
        });

        // enable/disable the button based on whether there is an undo history
        auto callback = [] (Views::TextButtonView *iButton, GUIJmbParam<UndoHistory> &iParam) {
          iButton->setMouseEnabled(iParam->hasActionHistory());
        };

        makeParamAware(button)->registerCallback<UndoHistory>(fState->fUndoHistory,
                                                              std::move(callback),
                                                              true);
        break;
      }

      case ESampleSplitterParamID::kResampleAction:
      {
        // we set a listener to handle what happens when the button is clicked
        button->setOnClickListener(processAction(SampleAction::Type::kResample));

        // we also make sure that the button is selected only when resampling is available
        auto cx = makeParamAware(button);
        cx->registerParam(fState->fSampleRate);
        cx->registerParam(fState->fCurrentSample);
        cx->registerListener([this] (Views::TextButtonView *iButton, ParamID iParamID) {
          iButton->setMouseEnabled(fState->fCurrentSample->hasSamples() &&
                                   *fState->fSampleRate != fState->fCurrentSample->getOriginalSampleRate());
        });
        cx->invokeAll();
        break;
      }

      default:
        break;
    }
  }
  else
  {
    auto label = dynamic_cast<VSTGUI::CTextLabel *>(iView);
    if(label)
    {
      switch(label->getTag())
      {
        case ESampleSplitterParamID::kSampleRate:
        {
          auto callback = [] (VSTGUI::CTextLabel *iLabel, GUIJmbParam<SampleRate> &iParam) {
            iLabel->setText(UTF8String(Steinberg::String().printf("%d", static_cast<int32>(*iParam))));
          };

          makeParamAware(label)->registerCallback<SampleRate>(fState->fSampleRate, std::move(callback), true);
          break;
        }

        default:
          // nothing to do
          break;
      }
    }
  }

  return iView;
}

//------------------------------------------------------------------------
// SampleEditController::processAction
//------------------------------------------------------------------------
Views::TextButtonView::OnClickListener SampleEditController::processAction(SampleAction::Type iActionType)
{
  Views::TextButtonView::OnClickListener listener =
    [this, iActionType] () -> void {
      auto action = SampleAction(iActionType);
      // we record the size of 1 slice before action
      auto size = computeSliceSizeInSamples();
      if(fState->fSampleMgr->executeAction(action))
      {
        // after action, we want to maintain the same size for 1 slice
        // for example if there was 16 slices and we cut 2 slices, we end up with 14 slices
        if(iActionType != SampleAction::Type::kResample)
          fNumSlices.update(computeNumSlices(size));
      }
    };

  return listener;
}


//------------------------------------------------------------------------
// SampleEditController::initButton
//------------------------------------------------------------------------
void SampleEditController::initButton(Views::TextButtonView *iButton,
                                      SampleAction::Type iActionType,
                                      bool iEnabledOnSelection)
{
  // we set a listener to handle what happens when the button is clicked
  iButton->setOnClickListener(processAction(iActionType));

  if(iEnabledOnSelection)
  {
    auto callback = [] (Views::TextButtonView *iButton, GUIJmbParam<SampleRange> &iParam) {
      iButton->setMouseEnabled(!iParam->isSingleValue());
    };

    // we register the callback to enable/disable the button based on the selection
    makeParamAware(iButton)->registerCallback<SampleRange>(fState->fWESelectedSampleRange,
                                                           std::move(callback),
                                                           true);
  }
}

//------------------------------------------------------------------------
// SampleEditController::createAction
//------------------------------------------------------------------------
void SampleEditController::registerParameters()
{
  fNumSlices = registerParam(fParams->fNumSlices, false);
}

//------------------------------------------------------------------------
// SampleEditController::computeSliceSizeInSamples
//------------------------------------------------------------------------
int32 SampleEditController::computeSliceSizeInSamples() const
{
  auto numSamples = fState->fCurrentSample->hasSamples() ? fState->fCurrentSample->getNumSamples() : 0;

  if(numSamples > 0)
  {
    DCHECK_F(fNumSlices->realValue() > 0);
    return static_cast<int32>(numSamples / fNumSlices->realValue());
  }

  return 0;
}

//------------------------------------------------------------------------
// SampleEditController::computeNumSlices
//------------------------------------------------------------------------
NumSlice SampleEditController::computeNumSlices(int32 iSliceSizeInSamples) const
{
  auto numSamples = fState->fCurrentSample->hasSamples() ? fState->fCurrentSample->getNumSamples() : 0;

  if(numSamples > 0 && iSliceSizeInSamples > 0)
  {
    return NumSlice{static_cast<NumSlice::real_type>(numSamples) / iSliceSizeInSamples};
  }
  else
    return NumSlice{DEFAULT_NUM_SLICES};
}


}