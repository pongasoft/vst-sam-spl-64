#include "PadController.h"
#include "PadView.h"

namespace pongasoft::VST::SampleSplitter::GUI {

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
    int index = static_cast<int>(pad->getControlTag() - ESampleSplitterParamID::kPad1);
    DCHECK_F(index >= 0 && index < NUM_PADS);
    fPads[index] = pad;
    int slice = *fPadBank * NUM_PADS + index;
    pad->setSlice(slice, slice < fNumSlices->intValue());
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
  fNumSlices = registerParam(fParams->fNumSlices);
  fPadBank = registerParam(fParams->fPadBank);
  fPlayingState = registerParam(fState->fPlayingState);
  fSelectedSlice = registerParam(fParams->fSelectedSlice, false);
  registerCallback<int>(fParams->fSelectedSliceViaMidi, [this](GUIVstParam<int> &iSelectedSliceViaMidi){
    if(fSelectedSlice != iSelectedSliceViaMidi && iSelectedSliceViaMidi < fNumSlices->intValue())
    {
      fSelectedSlice.copyValueFrom(iSelectedSliceViaMidi);
      // it is possible that the slice selected via MIDI is on a different bank => switch to it
      fPadBank = *iSelectedSliceViaMidi / NUM_PADS;
    }
  });
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
    int slice = *fPadBank * NUM_PADS + iPadIndex;
    pad->setSlice(slice, slice < fNumSlices->intValue());
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
    int slice = *fPadBank * NUM_PADS;
    for(auto pad : fPads)
    {
      if(pad)
        pad->setPercentPlayed(fPlayingState->fPercentPlayed[slice++]);
    }
  }
}

}