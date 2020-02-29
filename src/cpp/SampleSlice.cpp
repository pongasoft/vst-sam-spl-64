#include "SampleSlice.h"
#include "Model.h"

namespace pongasoft::VST::SampleSplitter {

//------------------------------------------------------------------------
// SampleSlice::getPercentPlayed
//------------------------------------------------------------------------
float SampleSlice::getPercentPlayed() const
{
  if(fState == EPlayingState::kPlaying)
  {
    auto numSlices = fSlicer.numSlices();
    if(numSlices > 0)
    {
      return static_cast<float>(fSlicer.numSlicesPlayed()) / numSlices;
    }
  }

  return PERCENT_PLAYED_NOT_PLAYING;
}

//------------------------------------------------------------------------
// SampleSlice::start
//------------------------------------------------------------------------
void SampleSlice::start(uint32 iStartFrame)
{
  fStartFrame = iStartFrame;
  fState = EPlayingState::kPlaying;
  fSlicer.start();
}

//------------------------------------------------------------------------
// SampleSlice::requestStop
//------------------------------------------------------------------------
EPlayingState SampleSlice::requestStop()
{
  if(fSlicer.requestEnd())
    fState = EPlayingState::kNotPlaying;

  return fState;
}

}