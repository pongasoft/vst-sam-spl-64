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
      case ESampleSplitterParamID::kNormalizeAction:
        button->setOnClickListener(process([] (SampleData *iData) -> tresult { return iData->normalize(); }));
        break;

      case ESampleSplitterParamID::kTrimAction:
        button->setOnClickListener(process([] (SampleData *iData) -> tresult { return iData->trim(); }));
        break;

      case ESampleSplitterParamID::kCutAction:
        button->setOnClickListener(process([this] (SampleData *iData) -> tresult {
          return iData->cut(static_cast<int32>(fState->fSelectedSampleRange->fFrom),
                            static_cast<int32>(fState->fSelectedSampleRange->fTo));
        }));
        break;

      case ESampleSplitterParamID::kCropAction:
        button->setOnClickListener(process([this] (SampleData *iData) -> tresult {
          return iData->crop(static_cast<int32>(fState->fSelectedSampleRange->fFrom),
                             static_cast<int32>(fState->fSelectedSampleRange->fTo));
        }));
        break;

      case ESampleSplitterParamID::kUndoAction:
        button->setOnClickListener(process([] (SampleData *iData) -> tresult { return iData->undo(); }));
        break;

      default:
        break;
    }
  }

  return iView;
}

//------------------------------------------------------------------------
// SampleEditController::process
//------------------------------------------------------------------------
Views::TextButtonView::OnClickListener SampleEditController::process(SampleEditController::ProcessingCallback iProcessingCallback)
{
  Views::TextButtonView::OnClickListener listener =
    [this, iProcessingCallback = std::move(iProcessingCallback)] () -> void {
      if(fState->fSampleData.updateIf([iProcessingCallback] (SampleData *iData) -> bool
                                      {
                                        return iProcessingCallback(iData) == kResultOk;
                                      })
        )
      {
        fState->broadcastSample();
      }
    };

  return listener;
}

}
}
}
}