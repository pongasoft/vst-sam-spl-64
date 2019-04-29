#include "SampleEditController.h"

#include <vstgui4/vstgui/lib/iviewlistener.h>
#include <vstgui4/vstgui/lib/controls/ctextlabel.h>

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

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
        initButton(button, SampleData::Action::Type::kNormalize0, false);
        break;

      case ESampleSplitterParamID::kNormalize3Action:
        initButton(button, SampleData::Action::Type::kNormalize3, false);
        break;

      case ESampleSplitterParamID::kNormalize6Action:
        initButton(button, SampleData::Action::Type::kNormalize6, false);
        break;

      case ESampleSplitterParamID::kTrimAction:
        initButton(button, SampleData::Action::Type::kTrim, false);
        break;

      case ESampleSplitterParamID::kCutAction:
        initButton(button, SampleData::Action::Type::kCut, true);
        break;

      case ESampleSplitterParamID::kCropAction:
        initButton(button, SampleData::Action::Type::kCrop, true);
        break;

      case ESampleSplitterParamID::kUndoAction:
      {
        // sets a listener to handle undo click
        button->setOnClickListener(std::bind(&SampleEditController::undoLastAction, this));

        // enable/disable the button based on whether there is an undo history
        auto callback = [] (Views::TextButtonView *iButton, GUIJmbParam<SampleData> &iParam) {
          iButton->setMouseEnabled(iParam->hasUndoHistory());
        };

        fState->registerConnectionFor(button)->registerCallback<SampleData>(fState->fSampleData, std::move(callback), true);
        break;
      }

      case ESampleSplitterParamID::kResampleAction:
      {
        // we set a listener to handle what happens when the button is clicked
        button->setOnClickListener(processAction(SampleData::Action::Type::kResample));

        // we also make sure that the button is selected only when resampling is available
        auto cx = fState->registerConnectionFor(button);
        cx->registerParam(fState->fSampleRate);
        cx->registerParam(fState->fSampleData);
        cx->registerParam(fState->fWESelectedSampleRange);
        cx->registerListener([this] (Views::TextButtonView *iButton, ParamID iParamID) {
          SampleInfo info;
          if(fState->fWESelectedSampleRange->isSingleValue() && fState->fSampleData->getSampleInfo(info) == kResultOk)
            iButton->setMouseEnabled(info.fSampleRate != fState->fSampleRate);
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
            iLabel->setText(UTF8String(String().printf("%d", static_cast<int32>(iParam))));
          };

          fState
            ->registerConnectionFor(label)
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
Views::TextButtonView::OnClickListener SampleEditController::processAction(SampleData::Action::Type iActionType)
{
  Views::TextButtonView::OnClickListener listener =
    [this, iActionType] () -> void {
      auto action = createAction(iActionType);
      if(fState->fSampleData.updateIf([&action] (SampleData *iData) -> bool
                                      {
                                        return iData->execute(action) == kResultOk;
                                      })
        )
      {
        fState->broadcastSample();
      }
    };

  return listener;
}


//------------------------------------------------------------------------
// SampleEditController::initButton
//------------------------------------------------------------------------
void SampleEditController::initButton(Views::TextButtonView *iButton,
                                      SampleData::Action::Type iActionType,
                                      bool iEnabledOnSelection)
{
  // we set a listener to handle what happens when the button is clicked
  iButton->setOnClickListener(processAction(iActionType));

  auto callback = [iEnabledOnSelection] (Views::TextButtonView *iButton, GUIJmbParam<SampleRange> &iParam) {
    iButton->setMouseEnabled(iEnabledOnSelection == !iParam->isSingleValue());
  };

  // we register the callback to enable/disable the button based on the selection
  fState->registerConnectionFor(iButton)->registerCallback<SampleRange>(fState->fWESelectedSampleRange,
                                                                        std::move(callback),
                                                                        true);
}

//------------------------------------------------------------------------
// SampleEditController::undoLastAction
//------------------------------------------------------------------------
void SampleEditController::undoLastAction()
{
  std::unique_ptr<SampleData::Action> result{};
  if(fState->fSampleData.updateIf([&result] (SampleData *iData) -> bool
                                  {
                                    result = iData->undo();
                                    return result != nullptr;
                                  })
    )
  {
    fState->broadcastSample();
  }
  if(result)
  {
    fState->fWESelectedSampleRange.update(result->fSelectedSampleRange);
    fOffsetPercent.setValue(result->fOffsetPercent);
    fZoomPercent.setValue(result->fZoomPercent);
  }

}

//------------------------------------------------------------------------
// SampleEditController::createAction
//------------------------------------------------------------------------
SampleData::Action SampleEditController::createAction(SampleData::Action::Type iActionType) const
{
  auto action = SampleData::Action{iActionType};
  action.fSelectedSampleRange = fState->fWESelectedSampleRange;
  action.fOffsetPercent = fOffsetPercent;
  action.fZoomPercent = fZoomPercent;
  action.fSampleRate = fState->fSampleRate;
 return action;
}

//------------------------------------------------------------------------
// SampleEditController::createAction
//------------------------------------------------------------------------
void SampleEditController::registerParameters()
{
  fOffsetPercent = registerParam(fParams->fWEOffsetPercent, false);
  fZoomPercent = registerParam(fParams->fWEZoomPercent, false);
}



}
}
}
}