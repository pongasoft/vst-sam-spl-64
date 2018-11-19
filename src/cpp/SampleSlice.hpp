#pragma once

#include "SampleSlice.h"

namespace pongasoft {
namespace VST {
namespace SampleSplitter {

//------------------------------------------------------------------------
// SampleSlice::play
//------------------------------------------------------------------------
template<typename SampleType>
EPlayingState SampleSlice::play(SampleBuffers32 &iSample, AudioBuffers<SampleType> &oAudioBuffers, bool iOverride)
{
  if(fState != EPlayingState::kPlaying)
    return fState;

  int32 newCurrent = fCurrent;
  EPlayingState newState = fState;

  auto numChannels = std::min(iSample.getNumChannels(), oAudioBuffers.getNumChannels());

  for(int32 c = 0; c < numChannels; c++)
  {
    int32 current = fCurrent;
    EPlayingState state = fState;

    auto channel = oAudioBuffers.getAudioChannel(c);
    if(!channel.isActive())
      continue;

    auto audioBuffer = channel.getBuffer(); // we know it is not null here
    auto sampleBuffer = iSample.getBuffer()[c];
    bool silent = true;

    for(int32 i = 0; i < oAudioBuffers.getNumSamples(); i++)
    {
      if(state != EPlayingState::kPlaying)
      {
        if(iOverride)
          audioBuffer[i] = 0;
        else
          silent = silent && isSilent(audioBuffer[i]);
        continue;
      }

      if(current < fStart || current >= fEnd)
      {
        current = getPlayStart();
        if(!fLoop || !isSelected())
        {
          if(iOverride)
            audioBuffer[i] = 0;
          else
            silent = silent && isSilent(audioBuffer[i]);
          state = EPlayingState::kDonePlaying;
          continue;
        }
      }

      if(iOverride)
        audioBuffer[i] = static_cast<SampleType>(sampleBuffer[current]);
      else
        audioBuffer[i] += static_cast<SampleType>(sampleBuffer[current]);

      silent = silent && isSilent(audioBuffer[i]);

      if(fReverse)
        current--;
      else
        current++;
    }

    channel.setSilenceFlag(silent);
    newCurrent = current;
    newState = state;
  }

  fCurrent = newCurrent;
  fState = newState;

  return fState;
}

}
}
}