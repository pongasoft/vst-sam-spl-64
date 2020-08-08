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
        initButton(button, SampleDataAction::Type::kNormalize0, false);
        break;

      case ESampleSplitterParamID::kNormalize3Action:
        initButton(button, SampleDataAction::Type::kNormalize3, false);
        break;

      case ESampleSplitterParamID::kNormalize6Action:
        initButton(button, SampleDataAction::Type::kNormalize6, false);
        break;

      case ESampleSplitterParamID::kTrimAction:
        initButton(button, SampleDataAction::Type::kTrim, false);
        break;

      case ESampleSplitterParamID::kCutAction:
        initButton(button, SampleDataAction::Type::kCut, true);
        break;

      case ESampleSplitterParamID::kCropAction:
        initButton(button, SampleDataAction::Type::kCrop, true);
        break;

      case ESampleSplitterParamID::kUndoAction:
      {
        // sets a listener to handle undo click
        button->setOnClickListener([this] { undoLastAction(); });

        // enable/disable the button based on whether there is an undo history
        auto callback = [] (Views::TextButtonView *iButton, GUIJmbParam<SampleDataMgr> &iParam) {
          iButton->setMouseEnabled(iParam->hasUndoHistory());
        };

        makeParamAware(button)->registerCallback<SampleDataMgr>(fState->fSampleDataMgr,
                                                                std::move(callback),
                                                                true);
        break;
      }

      case ESampleSplitterParamID::kRedoAction:
      {
        // sets a listener to handle redo click
        button->setOnClickListener([this] {
          if(fState->fSampleDataMgr.updateIf([] (SampleDataMgr *iMgr) -> bool {
            return iMgr->redoLastUndo();
          }))
          {
            fState->broadcastSample();
          }
        });

        // enable/disable the button based on whether there is an redo history
        auto callback = [] (Views::TextButtonView *iButton, GUIJmbParam<SampleDataMgr> &iParam) {
          iButton->setMouseEnabled(iParam->hasRedoHistory());
        };

        makeParamAware(button)->registerCallback<SampleDataMgr>(fState->fSampleDataMgr,
                                                                std::move(callback),
                                                                true);
        break;
      }

      case ESampleSplitterParamID::kClearHistoryAction:
      {
        // sets a listener to handle clearing history click
        button->setOnClickListener([this] {
          fState->fSampleDataMgr.updateIf([] (SampleDataMgr *iMgr) -> bool {
            return iMgr->clearActionHistory();
          });
        });

        // enable/disable the button based on whether there is an undo history
        auto callback = [] (Views::TextButtonView *iButton, GUIJmbParam<SampleDataMgr> &iParam) {
          iButton->setMouseEnabled(iParam->hasActionHistory());
        };

        makeParamAware(button)->registerCallback<SampleDataMgr>(fState->fSampleDataMgr,
                                                                std::move(callback),
                                                                true);
        break;
      }

      case ESampleSplitterParamID::kResampleAction:
      {
        // we set a listener to handle what happens when the button is clicked
        button->setOnClickListener(processAction(SampleDataAction::Type::kResample));

        // we also make sure that the button is selected only when resampling is available
        auto cx = makeParamAware(button);
        cx->registerParam(fState->fSampleRate);
        cx->registerParam(fState->fSampleData);
        cx->registerListener([this] (Views::TextButtonView *iButton, ParamID iParamID) {
          if(auto info = fState->fSampleData->getSampleInfo(); info)
            iButton->setMouseEnabled(info->fSampleRate != fState->fSampleRate);
          else
            iButton->setMouseEnabled(false);
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

          makeParamAware(label)
            ->registerCallback<SampleRate>(fState->fSampleRate, std::move(callback), true);
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
Views::TextButtonView::OnClickListener SampleEditController::processAction(SampleDataAction::Type iActionType)
{
  Views::TextButtonView::OnClickListener listener =
    [this, iActionType] () -> void {
      auto action = createAction(iActionType);
      // we record the size of 1 slice before action
      auto size = computeSliceSizeInSamples();
      if(fState->fSampleDataMgr.updateIf([&action] (SampleDataMgr *iMgr) -> bool
                                      {
                                        return iMgr->executeAction(action);
                                      })
        )
      {
        // we send the result of the action to RT
        fState->broadcastSample();

        // after action, we want to maintain the same size for 1 slice
        // for example if there was 16 slices and we cut 2 slices, we end up with 14 slices
        if(iActionType != SampleDataAction::Type::kResample)
          fNumSlices.update(computeNumSlices(size));
      }
    };

  return listener;
}


//------------------------------------------------------------------------
// SampleEditController::initButton
//------------------------------------------------------------------------
void SampleEditController::initButton(Views::TextButtonView *iButton,
                                      SampleDataAction::Type iActionType,
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
// SampleEditController::undoLastAction
//------------------------------------------------------------------------
void SampleEditController::undoLastAction()
{
  if(fState->fSampleDataMgr.updateIf([] (SampleDataMgr *iMgr) -> bool
                                  {
                                    return iMgr->undoLastAction();
                                  })
    )
  {
    fState->broadcastSample();
    auto result = fState->fSampleDataMgr->getLastUndoAction();
    if(result)
    {
      if(fState->fWESelectedSampleRange.update(result->fSelectedSampleRange))
        fState->fWESelectedSampleRange.broadcast();
      fNumSlices.setValue(result->fNumSlices);
      fOffsetPercent.setValue(result->fOffsetPercent);
      fZoomPercent.setValue(result->fZoomPercent);
    }
  }
}

//------------------------------------------------------------------------
// SampleEditController::createAction
//------------------------------------------------------------------------
SampleDataAction SampleEditController::createAction(SampleDataAction::Type iActionType) const
{
  auto action = SampleDataAction{iActionType};
  action.fNumSlices = *fNumSlices;
  action.fSelectedSampleRange = *fState->fWESelectedSampleRange;
  action.fOffsetPercent = *fOffsetPercent;
  action.fZoomPercent = *fZoomPercent;
  action.fSampleRate = *fState->fSampleRate;
 return action;
}

//------------------------------------------------------------------------
// SampleEditController::createAction
//------------------------------------------------------------------------
void SampleEditController::registerParameters()
{
  fOffsetPercent = registerParam(fParams->fWEOffsetPercent, false);
  fZoomPercent = registerParam(fParams->fWEZoomPercent, false);
  fNumSlices = registerParam(fParams->fNumSlices, false);
}

//------------------------------------------------------------------------
// SampleEditController::computeSliceSizeInSamples
//------------------------------------------------------------------------
int32 SampleEditController::computeSliceSizeInSamples() const
{
  auto numSamples = fState->fSampleData->getNumSamples(*fState->fSampleRate);

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
  auto numSamples = fState->fSampleData->getNumSamples(*fState->fSampleRate);

  if(numSamples > 0 && iSliceSizeInSamples > 0)
  {
    return NumSlice{static_cast<NumSlice::real_type>(numSamples) / iSliceSizeInSamples};
  }
  else
    return NumSlice{DEFAULT_NUM_SLICES};
}


}