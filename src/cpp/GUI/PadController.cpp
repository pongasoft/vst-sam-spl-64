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
    int slice = fPadBank * NUM_PADS + index;
    pad->setSlice(slice, slice < fNumSlices);
    pad->setPercentPlayed(fPlayingState->fPercentPlayed[slice]);
    return pad;
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
  fPlayingState = registerJmbParam(fState->fPlayingState);
}

//------------------------------------------------------------------------
// PadController::setSlice
//------------------------------------------------------------------------
void PadController::setSlice(int iPadIndex)
{
  DCHECK_F(iPadIndex >= 0 && iPadIndex < NUM_PADS);
  auto pad = fPads[iPadIndex];
  if(pad)
  {
    int slice = fPadBank * NUM_PADS + iPadIndex;
    pad->setSlice(slice, slice < fNumSlices);
  }
}

//------------------------------------------------------------------------
// PadController::onParameterChange
//------------------------------------------------------------------------
void PadController::onParameterChange(ParamID iParamID)
{
  if(iParamID == fNumSlices.getParamID() || iParamID == fPadBank.getParamID())
  {
    for(int i = 0; i < NUM_PADS; i++)
      setSlice(i);
  }

  if(iParamID == fPadBank.getParamID() || iParamID == fPlayingState.getParamID())
  {
    int slice = fPadBank * NUM_PADS;
    for(auto pad : fPads)
    {
      if(pad)
        pad->setPercentPlayed(fPlayingState->fPercentPlayed[slice++]);
    }
  }
}

}
}
}
}