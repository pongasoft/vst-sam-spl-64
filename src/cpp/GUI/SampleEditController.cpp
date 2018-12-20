#include <utility>

#include "SampleEditController.h"

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
        button->setOnClickListener(processAction(SampleData::Action::Type::kNormalize0));
        break;

      case ESampleSplitterParamID::kNormalize3Action:
        button->setOnClickListener(processAction(SampleData::Action::Type::kNormalize3));
        break;

      case ESampleSplitterParamID::kNormalize6Action:
        button->setOnClickListener(processAction(SampleData::Action::Type::kNormalize6));
        break;

      case ESampleSplitterParamID::kTrimAction:
        button->setOnClickListener(processAction(SampleData::Action::Type::kTrim));
        break;

      case ESampleSplitterParamID::kCutAction:
        button->setOnClickListener(processAction(SampleData::Action::Type::kCut));
        break;

      case ESampleSplitterParamID::kCropAction:
        button->setOnClickListener(processAction(SampleData::Action::Type::kCrop));
        break;

      case ESampleSplitterParamID::kUndoAction:
        button->setOnClickListener(std::bind(&SampleEditController::undoLastAction, this));
        break;

      default:
        break;
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
// SampleEditController::undoLastAction
//------------------------------------------------------------------------
void SampleEditController::undoLastAction()
{
  std::unique_ptr<SampleData::Action> result{};
  if(fState->fSampleData.updateIf([&result] (SampleData *iData) -> bool
                                  {
                                    result = std::move(iData->undo());
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
 return action;
}

//------------------------------------------------------------------------
// SampleEditController::createAction
//------------------------------------------------------------------------
void SampleEditController::registerParameters()
{
  fOffsetPercent = registerRawVstParam(fParams->fWEOffsetPercent, false);
  fZoomPercent = registerRawVstParam(fParams->fWEZoomPercent, false);
}



}
}
}
}