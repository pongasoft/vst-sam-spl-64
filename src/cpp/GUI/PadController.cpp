#include "PadController.h"
#include "PadView.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {
namespace GUI {

//------------------------------------------------------------------------
// PadController::verifyView
//------------------------------------------------------------------------
CView *PadController::verifyView(CView *iView,
                                 const UIAttributes &iAttributes,
                                 const IUIDescription * /* iDescription */)
{
  auto pad = dynamic_cast<PadView *>(iView);
  if(pad)
  {
    int index = pad->getControlTag() - ESampleSplitterParamID::kPad1;
    DCHECK_F(index >= 0 && index < NUM_PADS);
    fPads[index] = pad;
  }
  return iView;
}

//------------------------------------------------------------------------
// PadController::registerParameters
//------------------------------------------------------------------------
void PadController::registerParameters()
{
  DLOG_F(INFO, "PadController::registerParameters");
  fNumSlices = registerVstParam(fParams->fNumSlices);
  fPadBank = registerVstParam(fParams->fPadBank);
}

//------------------------------------------------------------------------
// PadController::setEnabled
//------------------------------------------------------------------------
void PadController::setEnabled(int iPadIndex)
{
  DCHECK_F(iPadIndex >= 0 && iPadIndex < NUM_PADS);
  auto pad = fPads[iPadIndex];
  if(pad)
  {
    int slice = fPadBank * NUM_PADS + iPadIndex;
    pad->setEnabled(slice < fNumSlices);
  }
}

//------------------------------------------------------------------------
// PadController::onParameterChange
//------------------------------------------------------------------------
void PadController::onParameterChange(ParamID iParamID)
{
  for(int i = 0; i < NUM_PADS; i++)
    setEnabled(i);
}

}
}
}
}