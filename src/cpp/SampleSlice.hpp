#pragma once

#include "SampleSlice.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

//------------------------------------------------------------------------
// SampleSlice::play
//------------------------------------------------------------------------
template<typename SampleType>
void SampleSlice::play(SampleBuffers32 &iSample, AudioBuffers<SampleType> &oAudioBuffers)
{
  if(!isSelected())
    return;

  auto numChannels = std::min(iSample.getNumChannels(), oAudioBuffers.getNumChannels());
  int32 newCurrent = fCurrent;
  for(int32 c = 0; c < numChannels; c++)
  {
    int32 current = fCurrent;
    auto audioBuffer = oAudioBuffers.getBuffer()[c];
    auto sampleBuffer = iSample.getBuffer()[c];
    for(int32 i = 0; i < oAudioBuffers.getNumSamples(); i++)
    {
      if(current >= fStart && current < fEnd)
        audioBuffer[i] = static_cast<SampleType>(sampleBuffer[current++]);
      else
        audioBuffer[i] = 0;
      //TODO handle silent flag
    }
    newCurrent = current;
  }
  fCurrent = newCurrent;
}

}
}
}