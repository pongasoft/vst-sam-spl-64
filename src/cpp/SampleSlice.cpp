#include "SampleSlice.h"
#include "Model.h"

namespace pongasoft::VST::SampleSplitter {

//------------------------------------------------------------------------
// SampleSlice::reset
//------------------------------------------------------------------------
void SampleSlice::reset(SampleBuffers32 const *iSample, int32 iStart, int32 iEnd)
{
  DCHECK_F(iSample->getNumChannels() > 0);

  auto leftChannel = iSample->getChannelBuffer(DEFAULT_LEFT_CHANNEL);
  auto rightChannel = iSample->getNumChannels() > 1 ? iSample->getChannelBuffer(DEFAULT_RIGHT_CHANNEL) : leftChannel;

  fLeftSlicer.reset(leftChannel, iStart, iEnd);
  fRightSlicer.reset(rightChannel, iStart, iEnd);
}

//------------------------------------------------------------------------
// SampleSlice::getPercentPlayed
//------------------------------------------------------------------------
float SampleSlice::getPercentPlayed() const
{
  if(fState == EPlayingState::kPlaying)
  {
    auto numSlices = fLeftSlicer.numSlices();
    if(numSlices > 0)
    {
      return static_cast<float>(fLeftSlicer.numSlicesPlayed()) / numSlices;
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
  fLeftSlicer.start();
  fRightSlicer.start();
}

//------------------------------------------------------------------------
// SampleSlice::requestStop
//------------------------------------------------------------------------
EPlayingState SampleSlice::requestStop()
{
  auto leftEnded = fLeftSlicer.requestEnd();
  auto rightEnded = fRightSlicer.requestEnd();

  if(leftEnded && rightEnded)
    fState = EPlayingState::kNotPlaying;

  return fState;
}


}