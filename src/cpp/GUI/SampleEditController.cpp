#include "SampleEditController.h"
#include <pongasoft/VST/GUI/Views/TextButtonView.h>

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
        button->setOnClickListener([] {
          DLOG_F(INFO, "Normalize");
        });
        break;

      case ESampleSplitterParamID::kTrimAction:
        button->setOnClickListener([] {
          DLOG_F(INFO, "Trim");
        });
        break;

      case ESampleSplitterParamID::kTruncateAction:
        button->setOnClickListener([] {
          DLOG_F(INFO, "Truncate");
        });
        break;

      default:
        break;
    }
  }

  return iView;
}

//------------------------------------------------------------------------
// SampleEditController::registerParameters
//------------------------------------------------------------------------
void SampleEditController::registerParameters()
{
  DLOG_F(INFO, "SampleEditController::registerParameters");
}

}
}
}
}