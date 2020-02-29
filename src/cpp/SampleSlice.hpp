#pragma once

#include "SampleSlice.h"

namespace pongasoft::VST::SampleSplitter {

//------------------------------------------------------------------------
// SampleSlice::play
//------------------------------------------------------------------------
template<typename SampleType>
EPlayingState SampleSlice::play(SampleBuffers32 const &iSample, AudioBuffers<SampleType> &oAudioBuffers, bool iOverride)
{
  if(fState != EPlayingState::kPlaying)
    return fState;

  // sanity check
  DCHECK_F(fSlicer.startIdx() >= 0 && fSlicer.startIdx() < iSample.getNumSamples());
  DCHECK_F(fSlicer.endIdx() >= 0 && fSlicer.endIdx() < iSample.getNumSamples());
  DCHECK_F(fSlicer.startIdx() <= fSlicer.endIdx());

  auto newSlicer = fSlicer;
  auto newState = fState;

  for(int32 c = 0; c < oAudioBuffers.getNumChannels(); c++)
  {
    auto state = fState;

    auto channel = oAudioBuffers.getAudioChannel(c);
    if(!channel.isActive())
      continue;

    auto audioBuffer = channel.getBuffer(); // we know it is not null here

    auto sampleBuffer = iSample.getChannelBuffer(c);
    if(!sampleBuffer)
    {
      // case when output is stereo but sample is mono => duplicate (left) channel
      if(oAudioBuffers.getNumChannels() == 2 && c == DEFAULT_RIGHT_CHANNEL)
        sampleBuffer = iSample.getChannelBuffer(DEFAULT_LEFT_CHANNEL);

      // still no sampleBuffer... skipping
      if(!sampleBuffer)
        continue;
    }

    bool silent = true;

    auto slicer = fSlicer;

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

      if(!slicer.next())
      {
        if(!fLoop || !isSelected())
        {
          if(iOverride)
            audioBuffer[i] = 0;
          else
            silent = silent && isSilent(audioBuffer[i]);
          state = EPlayingState::kDonePlaying;
          continue;
        }
        slicer.restart();
      }

      auto sample = static_cast<SampleType>(slicer.getSample(sampleBuffer));

      if(iOverride)
        audioBuffer[i] = static_cast<SampleType>(sample);
      else
        audioBuffer[i] += static_cast<SampleType>(sample);

      silent = silent && isSilent(audioBuffer[i]);
    }

    channel.setSilenceFlag(silent);
    newSlicer = slicer;
    newState = state;
  }

  fSlicer = newSlicer;
  fState = newState;

  return fState;
}

}
